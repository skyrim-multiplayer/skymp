/**
 * Replaces only ' and " symbols
 * @param jsString 
 * @returns 
 */
 export function escapeJs(jsString: string): string {
  return jsString.replace("'", "\'").replace('"', '\"');
}
