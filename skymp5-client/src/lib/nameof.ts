export function nameof<T>(key: keyof T): keyof T {
  return key;
}
