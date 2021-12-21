export function nameof<T>(key: keyof T): keyof T {
  return key;
}

export function escapeJs(jsString: string): string {
  return jsString.replace("'", "\'").replace('"', '\"');
}
