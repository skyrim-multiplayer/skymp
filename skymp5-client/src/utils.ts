import * as sp from 'skyrimPlatform'

export function nameof<T>(key: keyof T): keyof T {
  return key
}

/**
 * Replaces only ' and " symbols
 * @param jsString
 * @returns
 */
export function escapeJs(jsString: string): string {
  return jsString.replace("'", "'").replace('"', '"')
}

export function hasSweetPie(): boolean {
  const modCount = sp.Game.getModCount()
  for (let i = 0; i < modCount; ++i) {
    if (sp.Game.getModName(i).toLowerCase().includes('sweetpie')) {
      return true
    }
  }
  return false
}
