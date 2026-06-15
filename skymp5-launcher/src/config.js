/**
 * Launcher configuration — developer-only.
 *
 * apiUrl  – Base URL of the SkyMP backend.
 *           Overridden by the API_URL environment variable (set in .env for
 *           local dev, or as a real env var in a packaged/CI build).
 *           The available game servers are fetched from GET /api/servers
 *           at runtime so they never need a launcher rebuild to update.
 */
module.exports = {
  apiUrl: process.env.API_URL || 'https://api.skyrimroleplay.co.uk',

  // Application slug registered with Nexus Mods for the one-click SSO login
  // (the flow Vortex/MO2/Wabbajack use). Empty = SSO disabled; the launcher
  // falls back to paste-your-API-key. Request a slug from Nexus support,
  // then set it here (or via the NEXUS_APP_SLUG env var at build time).
  nexusAppSlug: process.env.NEXUS_APP_SLUG || '',
}
