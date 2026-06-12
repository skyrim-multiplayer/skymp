'use strict'

/**
 * Minimal FOMOD installer — runs ModuleConfig.xml unattended.
 *
 * Wabbajack never executes FOMODs (its modlists replay the author's choices
 * file-by-file); we don't have a compile step, so this engine installs with
 * the AUTHOR'S DEFAULTS: required files, then for every step/group the
 * Required/Recommended (or first usable) options, then conditional installs
 * evaluated against the flags those choices set. Every selection is logged
 * so the modpack author can verify the outcome once.
 *
 * Supported: requiredInstallFiles, installSteps/optionalFileGroups (all group
 * types), conditionFlags, conditionalFileInstalls, flag/file dependencies.
 */

const fs   = require('fs')
const path = require('path')
const { XMLParser } = require('fast-xml-parser')

let _log = (...args) => console.log('[fomod]', ...args)
function setLogger(fn) { _log = (...args) => fn('[fomod]', ...args) }

// ── Helpers ───────────────────────────────────────────────────────────────────

const arr = x => (x == null ? [] : Array.isArray(x) ? x : [x])

/** Case-insensitive resolve of a (possibly backslashed) relative path. */
function resolveCI(baseDir, relPath) {
  const parts = String(relPath).split(/[\\/]/).filter(Boolean)
  let cur = baseDir
  for (const part of parts) {
    let entries
    try { entries = fs.readdirSync(cur) } catch { return null }
    const hit = entries.find(e => e.toLowerCase() === part.toLowerCase())
    if (!hit) return null
    cur = path.join(cur, hit)
  }
  return cur
}

function copyRecursive(src, dst) {
  const stat = fs.statSync(src)
  if (stat.isDirectory()) {
    fs.mkdirSync(dst, { recursive: true })
    for (const entry of fs.readdirSync(src)) {
      copyRecursive(path.join(src, entry), path.join(dst, entry))
    }
  } else {
    fs.mkdirSync(path.dirname(dst), { recursive: true })
    fs.copyFileSync(src, dst)
  }
}

/** Find ModuleConfig.xml anywhere up to 3 levels deep. Returns abs path or null. */
function findModuleConfig(dir, depth = 0) {
  if (depth > 3) return null
  let entries
  try { entries = fs.readdirSync(dir, { withFileTypes: true }) } catch { return null }
  for (const e of entries) {
    if (!e.isDirectory() && e.name.toLowerCase() === 'moduleconfig.xml') return path.join(dir, e.name)
  }
  for (const e of entries) {
    if (e.isDirectory()) {
      const found = findModuleConfig(path.join(dir, e.name), depth + 1)
      if (found) return found
    }
  }
  return null
}

// ── Dependency evaluation ─────────────────────────────────────────────────────

/**
 * @param {object} dep        <dependencies> node (or moduleDependencies pattern)
 * @param {Map} flags         flag name -> value set by chosen plugins
 * @param {(file: string) => boolean} fileExists
 */
function evalDependencies(dep, flags, fileExists) {
  if (!dep) return true
  const operator = (dep['@_operator'] || 'And').toLowerCase()

  const results = []
  for (const f of arr(dep.flagDependency)) {
    const want = String(f['@_value'] ?? '')
    const have = String(flags.get(f['@_flag']) ?? '')
    results.push(have === want)
  }
  for (const f of arr(dep.fileDependency)) {
    const state  = String(f['@_state'] || 'Active').toLowerCase()
    const exists = fileExists(String(f['@_file'] || ''))
    results.push(state === 'missing' ? !exists : exists)  // Active/Inactive ≈ present
  }
  // Game/script-extender version checks: assume satisfied.
  for (const _ of arr(dep.gameDependency)) results.push(true)
  for (const _ of arr(dep.fommDependency)) results.push(true)
  for (const nested of arr(dep.dependencies)) {
    results.push(evalDependencies(nested, flags, fileExists))
  }

  if (results.length === 0) return true
  return operator === 'or' ? results.some(Boolean) : results.every(Boolean)
}

/** Resolve a plugin's effective type name (Required/Recommended/Optional/NotUsable/CouldBeUsable). */
function pluginType(plugin, flags, fileExists) {
  const td = plugin.typeDescriptor
  if (!td) return 'Optional'
  if (td.type) return td.type['@_name'] || 'Optional'
  const dt = td.dependencyType
  if (dt) {
    for (const pattern of arr(dt.patterns?.pattern)) {
      if (evalDependencies(pattern.dependencies, flags, fileExists)) {
        return pattern.type?.['@_name'] || 'Optional'
      }
    }
    return dt.defaultType?.['@_name'] || 'Optional'
  }
  return 'Optional'
}

// ── Install ───────────────────────────────────────────────────────────────────

/**
 * Run the FOMOD at stagingDir, installing into modDir.
 *
 * @param {string} stagingDir  Extracted archive contents
 * @param {string} modDir      Output mod folder (created)
 * @param {(file: string) => boolean} [fileExists]  Plugin lookup for fileDependency
 * @returns {{ installed: number, choices: string[] }}
 */
function install(stagingDir, modDir, fileExists = () => false) {
  const configPath = findModuleConfig(stagingDir)
  if (!configPath) throw new Error('No ModuleConfig.xml found')

  // Sources are relative to the fomod/ folder's parent.
  const sourceRoot = path.dirname(path.dirname(configPath))

  const parser = new XMLParser({
    ignoreAttributes: false,
    attributeNamePrefix: '@_',
    parseAttributeValue: false,
    isArray: (name) => ['installStep', 'group', 'plugin', 'pattern', 'file', 'folder', 'flag',
                        'flagDependency', 'fileDependency', 'gameDependency', 'dependencies'].includes(name),
  })
  const xml    = parser.parse(fs.readFileSync(configPath, 'utf8'))
  const module_ = xml.config || xml.moduleConfig || xml
  const flags   = new Map()
  const queue   = []  // { source, destination, priority }
  const choices = []

  const queueFiles = (node) => {
    for (const f of arr(node?.file))   queue.push({ source: f['@_source'], destination: f['@_destination'] ?? '', priority: parseInt(f['@_priority'] || '0', 10), isFile: true })
    for (const d of arr(node?.folder)) queue.push({ source: d['@_source'], destination: d['@_destination'] ?? '', priority: parseInt(d['@_priority'] || '0', 10), isFile: false })
  }

  // 1. Always-installed files
  queueFiles(module_.requiredInstallFiles)

  // 2. Install steps with default selections
  for (const step of arr(module_.installSteps?.installStep)) {
    if (!evalDependencies(step.visible?.dependencies || step.visible, flags, fileExists)) {
      _log(`step "${step['@_name']}" hidden — skipped`)
      continue
    }
    for (const group of arr(step.optionalFileGroups?.group)) {
      const type    = (group['@_type'] || 'SelectAny')
      const plugins = arr(group.plugins?.plugin)

      const typed = plugins.map(p => ({ p, t: pluginType(p, flags, fileExists) }))
      const usable = typed.filter(x => x.t !== 'NotUsable')

      let selected = []
      if (type === 'SelectAll') {
        selected = usable
      } else if (type === 'SelectExactlyOne') {
        selected = [usable.find(x => x.t === 'Required') || usable.find(x => x.t === 'Recommended') || usable[0]].filter(Boolean)
      } else if (type === 'SelectAtMostOne') {
        selected = [usable.find(x => x.t === 'Required') || usable.find(x => x.t === 'Recommended')].filter(Boolean)
      } else { // SelectAny / SelectAtLeastOne
        selected = usable.filter(x => x.t === 'Required' || x.t === 'Recommended')
        if (type === 'SelectAtLeastOne' && selected.length === 0 && usable.length > 0) selected = [usable[0]]
      }

      for (const { p } of selected) {
        choices.push(`${step['@_name'] || 'step'} / ${group['@_name'] || 'group'} → ${p['@_name']}`)
        queueFiles(p.files)
        for (const flag of arr(p.conditionFlags?.flag)) {
          flags.set(flag['@_name'], String(flag['#text'] ?? ''))
        }
      }
    }
  }

  // 3. Conditional installs against the flags we set
  for (const pattern of arr(module_.conditionalFileInstalls?.patterns?.pattern)) {
    if (evalDependencies(pattern.dependencies, flags, fileExists)) {
      queueFiles(pattern.files)
    }
  }

  // 4. Apply, low priority first so higher priorities overwrite
  queue.sort((a, b) => a.priority - b.priority)
  let installed = 0
  fs.mkdirSync(modDir, { recursive: true })
  for (const item of queue) {
    const src = resolveCI(sourceRoot, item.source)
    if (!src) { _log(`source missing in archive: ${item.source}`); continue }
    const dst = path.join(modDir, String(item.destination).replace(/[\\/]+/g, path.sep))
    copyRecursive(src, dst)
    installed++
  }

  for (const c of choices) _log(`chose: ${c}`)
  return { installed, choices }
}

module.exports = { setLogger, install, findModuleConfig }
