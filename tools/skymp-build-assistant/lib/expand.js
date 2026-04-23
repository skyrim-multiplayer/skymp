/**
 * Expand ${var} placeholders in a string using ctx.
 * Unknown placeholders are left unchanged.
 */
export function expandString(str, ctx) {
  if (typeof str !== 'string') {
    return str;
  }
  return str.replace(/\$\{([^}]+)\}/g, (_, key) => {
    const k = key.trim();
    if (Object.prototype.hasOwnProperty.call(ctx, k) && ctx[k] != null) {
      return String(ctx[k]);
    }
    return `\${${k}}`;
  });
}

export function expandDeep(value, ctx) {
  if (typeof value === 'string') {
    return expandString(value, ctx);
  }
  if (Array.isArray(value)) {
    return value.map((v) => expandDeep(v, ctx));
  }
  if (value && typeof value === 'object') {
    const out = {};
    for (const [k, v] of Object.entries(value)) {
      out[k] = expandDeep(v, ctx);
    }
    return out;
  }
  return value;
}
