export function nameof<T>(key: keyof T): keyof T {
  return key;
}

/**
 * Replaces only ' and " symbols
 * @param jsString 
 * @returns 
 */
export function escapeJs(jsString: string): string {
  return jsString.replace("'", "\'").replace('"', '\"');
}
