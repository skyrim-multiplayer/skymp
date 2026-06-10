/**
 * Launcher configuration — developer-only.
 *
 * apiUrl  – Base URL of the SkyRP backend.
 *           Overridden by the API_URL environment variable (set in .env for
 *           local dev, or as a real env var in a packaged/CI build).
 *           The available game servers are fetched from GET /api/servers
 *           at runtime so they never need a launcher rebuild to update.
 */
module.exports = {
  apiUrl: process.env.API_URL || 'https://api.skyrimroleplay.co.uk',
}
