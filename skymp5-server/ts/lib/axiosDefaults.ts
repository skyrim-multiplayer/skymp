/**
 * Shared axios request limits used across the server.
 *
 * All axios call sites MUST pass these (or stricter) values so that the
 * limit-enforcement code added in axios 1.15.1/1.15.2 actually takes effect.
 */

/** Timeout for standard outbound API calls (master server, Discord). */
export const DEFAULT_TIMEOUT_MS = 10_000;

/** Timeout for local-loopback probes (e.g. dev-server detection). */
export const LOCAL_PROBE_TIMEOUT_MS = 1_000;

/** Timeout for GitHub API calls (lower traffic, same network). */
export const GITHUB_API_TIMEOUT_MS = 5_000;

/** Maximum outgoing request body size for standard API calls (64 KiB). */
export const MAX_REQUEST_BODY_BYTES = 64 * 1024;

/** Maximum incoming response body size for standard API calls (256 KiB). */
export const MAX_RESPONSE_BODY_BYTES = 256 * 1024;

/**
 * Maximum incoming response body size for GitHub release payloads (2 MiB).
 * GitHub's /releases/latest JSON can include large asset listings.
 */
export const GITHUB_MAX_RESPONSE_BYTES = 2 * 1024 * 1024;
