'use strict'

/**
 * Generate data/modlist.json from a Nexus Mods collection revision.
 *
 *   node scripts/build-modlist.js <slug> <revision> [apikey]
 *   e.g. node scripts/collection-to-modlist.js lciswb 1
 *
 * The API key comes from the third argument or the NEXUS_API_KEY env var
 * (your personal key from https://next.nexusmods.com/settings/api-keys).
 */

const https = require('https')
const fs    = require('fs')
const path  = require('path')

const [slug, revisionArg, keyArg] = process.argv.slice(2)
const apiKey = keyArg || process.env.NEXUS_API_KEY

if (!slug || !revisionArg) {
  console.error('Usage: node scripts/collection-to-modlist.js <slug> <revision> [apikey]')
  process.exit(1)
}
if (!apiKey) {
  console.error('No API key: pass it as the third argument or set NEXUS_API_KEY.')
  process.exit(1)
}

const QUERY = `
query ($slug: String!, $revision: Int) {
  collectionRevision(slug: $slug, revision: $revision, viewAdultContent: true) {
    externalResources { name resourceUrl }
    modFiles {
      optional
      file {
        name
        version
        mod { modId name version summary }
      }
    }
  }
}`

function graphql(query, variables) {
  const body = JSON.stringify({ query, variables })
  return new Promise((resolve, reject) => {
    const req = https.request({
      hostname: 'api.nexusmods.com',
      path:     '/v2/graphql',
      method:   'POST',
      headers: {
        'Content-Type':   'application/json',
        'Content-Length': Buffer.byteLength(body),
        apikey:           apiKey,
        'User-Agent':     'SkyRP-Backend/1.0.0',
      },
    }, res => {
      let data = ''
      res.on('data', c => { data += c })
      res.on('end', () => {
        if (res.statusCode !== 200) {
          return reject(new Error(`Nexus GraphQL HTTP ${res.statusCode}: ${data.slice(0, 300)}`))
        }
        try {
          const json = JSON.parse(data)
          if (json.errors) return reject(new Error(`GraphQL errors: ${JSON.stringify(json.errors)}`))
          resolve(json.data)
        } catch (err) {
          reject(new Error(`Bad JSON from Nexus: ${err.message}`))
        }
      })
    })
    req.on('error', reject)
    req.setTimeout(20_000, () => { req.destroy(); reject(new Error('Request timed out')) })
    req.write(body)
    req.end()
  })
}

async function main() {
  console.log(`Fetching collection ${slug} revision ${revisionArg}…`)
  const data = await graphql(QUERY, { slug, revision: parseInt(revisionArg, 10) })

  const rev = data.collectionRevision
  if (!rev) throw new Error('Collection revision not found (check slug/revision, and that it is published).')

  // One entry per mod — collections list files, and a mod can contribute several.
  // A mod is "required" if ANY of its files is non-optional.
  const byModId = new Map()
  for (const mf of rev.modFiles || []) {
    const mod = mf.file?.mod
    if (!mod?.modId) continue
    const existing = byModId.get(mod.modId)
    if (existing) {
      existing.required = existing.required || !mf.optional
    } else {
      byModId.set(mod.modId, {
        name:     mod.name,
        version:  mod.version || mf.file.version || '',
        required: !mf.optional,
        enabled:  true,
        source:   'nexus',
        nexusId:  mod.modId,
      })
    }
  }

  const modlist = [
    { name: 'SkyMP Client', required: true, enabled: true, source: 'backend' },
    ...[...byModId.values()].sort((a, b) => a.name.localeCompare(b.name)),
  ]

  const outPath = path.join(__dirname, '..', 'data', 'modlist.json')
  fs.writeFileSync(outPath, JSON.stringify(modlist, null, 2) + '\n')
  console.log(`Wrote ${modlist.length} entries to ${outPath}`)

  const external = rev.externalResources || []
  if (external.length > 0) {
    console.log('\nExternal (non-Nexus) resources in this collection — the launcher')
    console.log('cannot auto-install these; ship them via the client files zip or')
    console.log('document them for players:')
    for (const r of external) console.log(`  - ${r.name}: ${r.resourceUrl}`)
  }
}

main().catch(err => {
  console.error('FAILED:', err.message)
  process.exit(1)
})
