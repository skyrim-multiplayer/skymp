const SUPPORTED_LANGUAGES = ['de', 'en', 'es', 'fr', 'it', 'ru'] as const;
const LANGUAGE_STORAGE_KEY = 'skymp.language';

export type SupportedLanguage = (typeof SUPPORTED_LANGUAGES)[number];

export const detectLanguage = (rawLanguage?: string): SupportedLanguage => {
  if (!rawLanguage) {
    return 'en';
  }

  const language = rawLanguage.toLowerCase().split('-')[0];

  return SUPPORTED_LANGUAGES.includes(language as SupportedLanguage)
    ? (language as SupportedLanguage)
    : 'en';
};

export const getStoredRuntimeLanguage = (): SupportedLanguage | null => {
  if (typeof window === 'undefined' || !window.localStorage) {
    return null;
  }

  try {
    const raw = window.localStorage.getItem(LANGUAGE_STORAGE_KEY);
    return raw ? detectLanguage(raw) : null;
  } catch {
    return null;
  }
};

export const persistRuntimeLanguage = (language: string): SupportedLanguage => {
  const normalized = detectLanguage(language);

  if (typeof window !== 'undefined' && window.localStorage) {
    try {
      window.localStorage.setItem(LANGUAGE_STORAGE_KEY, normalized);
    } catch {
      // ignore storage write failures
    }
  }

  return normalized;
};

export const detectRuntimeLanguage = (): SupportedLanguage => {
  const stored = getStoredRuntimeLanguage();
  if (stored) {
    return stored;
  }

  if (typeof navigator === 'undefined') {
    return 'en';
  }

  return detectLanguage(navigator.language);
};
