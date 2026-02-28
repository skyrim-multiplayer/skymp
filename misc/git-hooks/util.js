/**
 * @param {import('child_process').SpawnSyncReturns<string>} child
 */
export function ensureCleanExit(child) {
  if (child.error) {
    throw child.error;
  }
  if (child.signal) {
    throw new Error(`child terminated by signal: ${child.signal}`);
  }
  if (child.status !== 0) {
    throw new Error(`child exited with code ${child.status}`);
  }
  return child;
}
