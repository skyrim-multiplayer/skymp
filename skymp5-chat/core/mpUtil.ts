export function safeCall<T>(fn: () => T, fallback: T): T {
  try {
    return fn()
  } catch {
    return fallback
  }
}

export function safeSet(mp: { set(formId: number, propertyName: string, value: unknown): void }, formId: number, propertyName: string, value: unknown): boolean {
  try {
    mp.set(formId, propertyName, value)
    return true
  } catch (err: any) {
    console.error(`[mpUtil] failed to set ${propertyName} on ${formId}: ${err?.message ?? String(err)}`)
    return false
  }
}
