// Rewrites raw numeric form ids in MongoDB after the server learned to index
// ESL / light plugins the way the game runtime does.
//
// Before: every plugin got a flat byte index (0,1,2,...) regardless of ESL.
// After:  full plugins are numbered among themselves; light plugins move to
//         0xFE | slot<<12 | localId.
// FormDesc strings ("1a26f:Skyrim.esm") are filename-keyed and need no change.
//
// Run with the GAME SERVER STOPPED. Dry run by default.
//   node misc/migrate-formid-space.js
//   node misc/migrate-formid-space.js --apply
//
// Needs the mongodb driver: it resolves from server-manager/node_modules.

'use strict'

const fs = require('fs')
const path = require('path')

const REPO = path.dirname(__dirname)
const SETTINGS = path.join(REPO, 'build', 'dist', 'server', 'server-settings.json')
const SCHEMA_VERSION = 1
const APPLY = process.argv.includes('--apply')

function loadMongo() {
  for (const dir of ['server-manager', 'skymp5-backend']) {
    try { return require(path.join(REPO, dir, 'node_modules', 'mongodb')) } catch {}
  }
  try { return require('mongodb') } catch {}
  throw new Error('mongodb driver not found - run "npm install" in server-manager')
}

// TES4 header: type(4) dataSize(4) flags(4). Light plugin = flags & 0x200.
function isLightPlugin(file) {
  const fd = fs.openSync(file, 'r')
  try {
    const buf = Buffer.alloc(12)
    if (fs.readSync(fd, buf, 0, 12, 0) < 12) return false
    if (buf.toString('latin1', 0, 4) !== 'TES4') {
      throw new Error(`${file} is not a plugin (no TES4 header)`)
    }
    return (buf.readUInt32LE(8) & 0x200) !== 0
  } finally {
    fs.closeSync(fd)
  }
}

// old flat index -> new slot, for every plugin in loadOrder
function buildMap(loadOrder) {
  const map = []
  let nextFull = 0, nextLight = 0
  loadOrder.forEach((p, flatIdx) => {
    const light = isLightPlugin(p)
    map[flatIdx] = light
      ? { light: true, index: nextLight++, name: path.basename(p) }
      : { light: false, index: nextFull++, name: path.basename(p) }
  })
  return map
}

const hex = v => '0x' + (v >>> 0).toString(16).padStart(8, '0')

function remap(v, map) {
  if (!Number.isInteger(v) || v < 0) return v
  const hb = v >>> 24
  if (hb === 0xfe || hb === 0xff) return v // already light-encoded, or dynamic
  const slot = map[hb]
  if (!slot) throw new Error(`no plugin at old flat index ${hex(v)} (high byte 0x${hb.toString(16)})`)
  if (!slot.light) return (((slot.index << 24) >>> 0) | (v & 0x00ffffff)) >>> 0
  const short = v & 0x00ffffff
  if (short > 0xfff) {
    throw new Error(`${hex(v)} targets light plugin ${slot.name} but its local id exceeds 12 bits`)
  }
  return (0xfe000000 | (slot.index << 12) | short) >>> 0
}

// Every raw-numeric form id field written by MpChangeForm::ToJson
function walkDoc(doc, map, hits) {
  const num = (obj, key, where) => {
    const before = obj[key]
    if (!Number.isInteger(before)) return
    const after = remap(before, map)
    if (after !== before) {
      obj[key] = after
      hits.push({ where, before, after })
    }
  }
  const entries = (inv, where) => {
    if (!inv || !Array.isArray(inv.entries)) return
    inv.entries.forEach((e, i) => {
      num(e, 'baseId', `${where}[${i}].baseId`)
      num(e, 'enchantmentId', `${where}[${i}].enchantmentId`)
      num(e, 'poisonId', `${where}[${i}].poisonId`)
    })
  }
  entries(doc.inv, 'inv.entries')
  if (doc.equipmentDump) {
    entries(doc.equipmentDump.inv, 'equipmentDump.inv.entries')
    for (const k of ['leftSpell', 'rightSpell', 'voiceSpell', 'instantSpell']) {
      num(doc.equipmentDump, k, `equipmentDump.${k}`)
    }
  }
  if (Array.isArray(doc.learnedSpells)) {
    doc.learnedSpells.forEach((v, i) => {
      const after = remap(v, map)
      if (after !== v) { doc.learnedSpells[i] = after; hits.push({ where: `learnedSpells[${i}]`, before: v, after }) }
    })
  }
  if (Array.isArray(doc.effects)) {
    doc.effects.forEach((e, i) => {
      if (e) num(e, 'effectId', `effects[${i}].effectId`)
    })
  }
  // appearanceDump.tints[].argb are colors, not form ids - deliberately skipped
  if (doc.appearanceDump) {
    num(doc.appearanceDump, 'raceId', 'appearanceDump.raceId')
    num(doc.appearanceDump, 'headTextureSetId', 'appearanceDump.headTextureSetId')
    if (Array.isArray(doc.appearanceDump.headpartIds)) {
      doc.appearanceDump.headpartIds.forEach((v, i) => {
        const after = remap(v, map)
        if (after !== v) {
          doc.appearanceDump.headpartIds[i] = after
          hits.push({ where: `appearanceDump.headpartIds[${i}]`, before: v, after })
        }
      })
    }
  }
  return hits
}

async function main() {
  const settings = JSON.parse(fs.readFileSync(SETTINGS, 'utf8'))
  if (!Array.isArray(settings.loadOrder) || !settings.loadOrder.length) {
    throw new Error('server-settings.json has no loadOrder')
  }
  const uri = settings.databaseUri
  const dbName = settings.databaseName || 'skymp'
  if (settings.databaseDriver !== 'mongodb' || !uri) {
    throw new Error('server-settings.json is not on the mongodb driver')
  }

  const map = buildMap(settings.loadOrder)
  console.log(`load order: ${map.length} plugins, ${map.filter(m => m.light).length} light\n`)
  map.forEach((m, i) => {
    const before = hex(i << 24)
    const after = m.light ? `0xFE${m.index.toString(16).padStart(3, '0')}xxx` : hex(m.index << 24)
    if (before !== after.replace('xxx', '000')) {
      console.log(`  ${String(i).padStart(2)} ${m.name.padEnd(46)} ${before} -> ${after}`)
    }
  })

  const { MongoClient } = loadMongo()
  const client = new MongoClient(uri, { serverSelectionTimeoutMS: 5000 })
  await client.connect()
  try {
    const db = client.db(dbName)
    const meta = db.collection('schemaMeta')
    const existing = await meta.findOne({ _id: 'formIdSpace' })
    if (existing && existing.version >= SCHEMA_VERSION) {
      console.log(`\nAlready migrated (version ${existing.version}, ${existing.appliedAt}). Nothing to do.`)
      return
    }

    const coll = db.collection('changeForms')
    const docs = await coll.find({ _fidv: { $ne: SCHEMA_VERSION } }).toArray()
    console.log(`\nscanning ${docs.length} changeForms...\n`)

    let changedDocs = 0, changedValues = 0
    const ops = []
    for (const doc of docs) {
      const hits = walkDoc(doc, map, [])
      if (hits.length) {
        changedDocs++
        changedValues += hits.length
        for (const h of hits) {
          console.log(`  ${String(doc.formDesc).padEnd(22)} ${h.where.padEnd(40)} ${hex(h.before)} -> ${hex(h.after)}`)
        }
      }
      // Only rewrite documents that actually changed. Writing back untouched
      // subtrees would revert any save the server made after we read them.
      if (APPLY && hits.length) {
        const set = { _fidv: SCHEMA_VERSION }
        if (doc.inv) set.inv = doc.inv
        if (doc.equipmentDump) set.equipmentDump = doc.equipmentDump
        if (doc.learnedSpells) set.learnedSpells = doc.learnedSpells
        if (doc.appearanceDump) set.appearanceDump = doc.appearanceDump
        if (doc.effects) set.effects = doc.effects
        ops.push({ updateOne: { filter: { _id: doc._id }, update: { $set: set } } })
      }
    }

    console.log(`\n${changedValues} value(s) in ${changedDocs} document(s) would change.`)

    if (!APPLY) {
      console.log('\nDRY RUN - nothing written. Re-run with --apply once the numbers look right.')
      return
    }
    // Back up before writing so a rollback is one rename
    const backup = `changeForms_bak_${Date.now()}`
    await coll.aggregate([{ $match: {} }, { $out: backup }]).toArray()
    console.log(`backed up to ${backup}`)

    for (let i = 0; i < ops.length; i += 500) {
      await coll.bulkWrite(ops.slice(i, i + 500), { ordered: false })
    }
    // Mark the untouched documents too, so a re-run does not rescan them
    await coll.updateMany({ _fidv: { $ne: SCHEMA_VERSION } },
                          { $set: { _fidv: SCHEMA_VERSION } })
    await meta.updateOne(
      { _id: 'formIdSpace' },
      { $set: { version: SCHEMA_VERSION, appliedAt: new Date().toISOString(), plugins: map.length } },
      { upsert: true }
    )
    console.log(`\napplied to ${ops.length} document(s).`)
  } finally {
    await client.close()
  }
}

main().catch(err => { console.error('\nFAILED:', err.message); process.exit(1) })
