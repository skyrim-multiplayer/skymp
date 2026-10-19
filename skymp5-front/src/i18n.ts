import i18n from 'i18next';
import { initReactI18next } from 'react-i18next';

import de from './locales/de.json';
import en from './locales/en.json';
import es from './locales/es.json';
import fr from './locales/fr.json';
import it from './locales/it.json';
import itemDe from './locales/items/item_de.json';
import itemEn from './locales/items/item_en.json';
import itemEs from './locales/items/item_es.json';
import itemFr from './locales/items/item_fr.json';
import itemIt from './locales/items/item_it.json';
import itemRu from './locales/items/item_ru.json';
import ru from './locales/ru.json';
import { detectRuntimeLanguage } from './utils/i18nLanguage';

i18n.use(initReactI18next).init({
  resources: {
    en: {
      translation: {
        ...en,
        items: itemEn
      }
    },
    ru: {
      translation: {
        ...ru,
        items: itemRu
      }
    },
    de: {
      translation: {
        ...de,
        items: itemDe
      }
    },
    es: {
      translation: {
        ...es,
        items: itemEs
      }
    },
    fr: {
      translation: {
        ...fr,
        items: itemFr
      }
    },
    it: {
      translation: {
        ...it,
        items: itemIt
      }
    }
  },
  lng: detectRuntimeLanguage(),
  fallbackLng: 'en',
  supportedLngs: ['de', 'en', 'es', 'fr', 'it', 'ru'],
  load: 'languageOnly',
  interpolation: {
    escapeValue: false
  }
});

export default i18n;
