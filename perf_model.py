"""
SkyMP Performance Projection Model v2
======================================
Calibrated against real benchmark data from unit/ParallelBenchmark.cpp
on a Ryzen 9950X3D (16 physical cores, 32 threads).

The model decomposes a server tick into three serial phases:

  1. INGEST:  Linear in N. Building the snapshot from incoming packets.
  2. TASK:    N*neighbors work. Relay-decision computation. This is what
             gets parallelized across worker threads.
  3. JOIN:    N*neighbors work. Merging task results and emitting relays.
             Always serial on the main thread.

In the INLINE path (no parallel framework), phases 2 and 3 are fused
and the per-edge cost is different from the parallel path.

Constants derived from benchmark breakdowns:
  - 400 players: ingest=165.2, tasksum=851, join=282, parallel_wall=117
  - 150 players: ingest=61.9,  tasksum=121, join=48,  parallel_wall=39
"""

import math

# ─── Calibration constants (Ryzen 9950X3D baseline, IPC = 1.0) ──────────

# Phase 1: Snapshot ingest. Linear in N.
#   Measured: 165.2/400 = 0.413, 61.9/150 = 0.413
C_INGEST = 0.413

# Phase 2: Task work per relay edge (N * neighbors).
#   Measured: 851/(400*400) = 0.00532, 121/(150*150) = 0.00538
C_TASK_EDGE = 0.00535

# Phase 3: Join cost per relay edge in the parallel path.
#   Measured: 282/160000 = 0.00176, 48/22500 = 0.00213
#   Use geometric mean: sqrt(0.00176 * 0.00213) = 0.00194
C_JOIN_EDGE = 0.00194

# Inline fused relay cost per edge (no parallel framework).
#   Derived from inline totals: fit A*N + B*N^2 to measured data
#   100 players: 101.6, 400 players: 1173.6
#   Solving with C_FIXED_OVERHEAD=4.0:
#     101.6 = 4 + 100A + 10000B
#     1173.6 = 4 + 400A + 160000B
#   → B = 0.00639, A = 0.337
C_INLINE_LINEAR = 0.337    # per-actor fixed cost
C_INLINE_EDGE   = 0.00639  # per-edge cost (fused task+relay)

# Thread synchronization barrier (condvar round-trip + scheduler overhead)
#   Measured: parallel_wall - tasksum/active_threads
#   400p: 117 - 851/8 = 10.6 us,  150p: 39 - 121/5 = 14.8 us
C_BARRIER_BASE = 12.0  # us, average

# Fixed per-tick overhead not captured by the per-actor/per-edge model.
# Covers PartOne::Tick housekeeping, snapshot allocation, etc.
# Visible primarily at low N where it's a large fraction of the tick.
C_FIXED_OVERHEAD = 4.0  # us


class HardwareProfile:
    def __init__(self, name, physical_cores, ipc, barrier_scale):
        """
        physical_cores: Number of physical CPU cores (NOT vCPUs/logical).
                        The C++ Normalize() does hardware_concurrency()/2.
        ipc:            Single-thread IPC relative to baseline (Ryzen 9950X3D = 1.0).
        barrier_scale:  Multiplier on barrier cost relative to baseline.
                        Higher for multi-socket, NUMA, or cross-CCD architectures.
        """
        self.name = name
        self.physical_cores = physical_cores
        self.ipc = ipc
        self.barrier_scale = barrier_scale


PROFILES = [
    #                                                   phys   IPC   barrier
    HardwareProfile("Ryzen 9950X3D (Local Baseline)",     16,  1.00,  1.0),
    HardwareProfile("AWS c6i.2xlarge (Ice Lake, 4p/8v)",   4,  0.75,  1.5),
    HardwareProfile("AWS c6i.4xlarge (Ice Lake, 8p/16v)",  8,  0.75,  1.8),
    HardwareProfile("AWS c7g.2xlarge (Graviton3, 8p)",     8,  0.90,  1.2),
    HardwareProfile("AWS c7g.4xlarge (Graviton3, 16p)",   16,  0.90,  1.4),
    HardwareProfile("AWS c7a.8xlarge (Genoa, 16p/32v)",   16,  1.05,  2.0),
]


def compute_neighbors(players, interest_mgmt):
    """How many neighbors each actor checks for relay decisions."""
    if interest_mgmt:
        # InterestManager caps full-rate relays based on distance.
        # Measured: 400 players in one chunk → ~165 neighbors receive relays.
        return min(players, 165)
    return players


def simulate_inline(players, hw, interest_mgmt=False):
    """Predict tick cost for the legacy inline path (no OffloadDispatcher)."""
    neighbors = compute_neighbors(players, interest_mgmt)
    edges = players * neighbors

    linear = C_INLINE_LINEAR / hw.ipc
    edge   = C_INLINE_EDGE / hw.ipc

    return C_FIXED_OVERHEAD + linear * players + edge * edges


def simulate_parallel(players, hw, interest_mgmt=True, min_shard_micros=20,
                      worker_override=None, verbose=False):
    """Predict tick cost with the parallel OffloadDispatcher."""
    neighbors = compute_neighbors(players, interest_mgmt)
    edges = players * neighbors

    c_ingest    = C_INGEST / hw.ipc
    c_task_edge = C_TASK_EDGE / hw.ipc
    c_join_edge = C_JOIN_EDGE / hw.ipc
    barrier     = C_BARRIER_BASE * hw.barrier_scale

    # 1. Ingest (serial)
    t_ingest = C_FIXED_OVERHEAD + c_ingest * players

    # 2. Task work (parallelizable)
    t_task_total = c_task_edge * edges

    # Auto worker count: min(physical - 1, kMaxAutoWorkerThreads=8)
    if worker_override is not None:
        workers = worker_override
    else:
        workers = min(hw.physical_cores - 1, 8)
    workers = max(workers, 1)

    # Shard count: estimated_work / min_shard_micros, capped at 2*workers
    shards = max(1, int(t_task_total / min_shard_micros))
    shards = min(shards, workers * 2)
    active = min(workers, shards)

    if active > 1 and players >= 100:  # minActorsToOffload default
        # The barrier cost is paid once for the round-trip, but there is also a small
        # serial dispatch overhead (waking workers, pulling from the queue) per shard.
        # Derived empirically to match the 20us optimum at 150 players.
        per_shard_overhead = 1.5 * hw.barrier_scale
        t_parallel_wall = (t_task_total / active) + barrier + (shards * per_shard_overhead)
    else:
        t_parallel_wall = t_task_total  # runs inline, no barrier

    # 3. Join (serial)
    t_join = c_join_edge * edges

    if verbose:
        total = t_ingest + t_parallel_wall + t_join
        return total, shards, active

    return t_ingest + t_parallel_wall + t_join


def find_max_players(simulate_fn, hw, target_ms=16.66, **kwargs):
    """Binary search for the maximum player count within the tick budget."""
    lo, hi, best = 10, 50000, 10
    while lo <= hi:
        mid = (lo + hi) // 2
        if simulate_fn(mid, hw, **kwargs) / 1000.0 <= target_ms:
            best = mid
            lo = mid + 1
        else:
            hi = mid - 1
    return best


def optimize_parameters(hw, players=400):
    best_cost = float('inf')
    best_shard = 20
    best_workers = 8

    # Ensure we cap workers search space up to the hardware's physical cores
    max_workers_to_test = min(hw.physical_cores, 32)

    for shard_micros in range(5, 105, 5):
        for test_workers in range(1, max_workers_to_test + 1):
            cost = simulate_parallel(players, hw, interest_mgmt=True,
                                     min_shard_micros=shard_micros,
                                     worker_override=test_workers)
            if cost < best_cost:
                best_cost = cost
                best_shard = shard_micros
                best_workers = test_workers
                
    return best_shard, best_workers, best_cost
def validate_against_benchmarks():
    """Compare model predictions to actual benchmark measurements."""
    hw = PROFILES[0]  # Ryzen baseline

    actuals_inline  = {25: 15.4, 50: 35.0, 100: 101.6, 150: 205.8, 250: 484.3, 400: 1173.6}
    actuals_parallel = {25: 18.3, 50: 40.4, 100: 100.6, 150: 148.9, 250: 296.2, 400: 564.2}

    print("Model Validation Against Benchmark Data")
    print("=" * 65)
    print(f"  {'players':>8}  {'actual':>10}  {'predicted':>10}  {'error':>8}  path")
    print(f"  {'-'*58}")

    max_err_inline = 0
    for n, actual in sorted(actuals_inline.items()):
        pred = simulate_inline(n, hw, interest_mgmt=False)
        err = (pred - actual) / actual * 100
        max_err_inline = max(max_err_inline, abs(err))
        print(f"  {n:>8}  {actual:>10.1f}  {pred:>10.1f}  {err:>+7.1f}%  inline")

    print()
    max_err_par = 0
    for n, actual in sorted(actuals_parallel.items()):
        pred, shards, active = simulate_parallel(n, hw, interest_mgmt=False, verbose=True)
        err = (pred - actual) / actual * 100
        max_err_par = max(max_err_par, abs(err))
        print(f"  {n:>8}  {actual:>10.1f}  {pred:>10.1f}  {err:>+7.1f}%  parallel  ({shards} shards, {active} active)")

    # Actual shard counts from benchmark: 25:1, 50:1, 100:2, 150:5, 250:17, 400:18
    print(f"  Actual shard counts:  25:1  50:1  100:2  150:5  250:17  400:18")
    print(f"\n  Max error: inline {max_err_inline:.1f}%, parallel {max_err_par:.1f}%\n")


def project_hardware():
    """Project maximum player counts across hardware profiles."""
    print("\nHardware Scaling Projections")
    print("=" * 80)
    print(f"  {'Hardware':<40} {'Cores':>5} {'Inline Max':>11} {'Par+IM Max':>11} {'@400 par':>10}")
    print(f"  {'-'*77}")

    for hw in PROFILES:
        max_inline = find_max_players(simulate_inline, hw, interest_mgmt=False)
        max_par_im = find_max_players(simulate_parallel, hw, interest_mgmt=True)
        cost_400   = simulate_parallel(400, hw, interest_mgmt=True)
        opt_shard, opt_workers, opt_cost = optimize_parameters(hw, 400)
        
        print(f"  {hw.name:<40} {hw.physical_cores:>5} {max_inline:>9} p {max_par_im:>9} p {cost_400/1000:>8.2f} ms")
        print(f"      -> Optimal @400: shard={opt_shard}us, workers={opt_workers}, cost={opt_cost/1000:.2f}ms")

    print()


def main():
    validate_against_benchmarks()
    project_hardware()


if __name__ == "__main__":
    main()
