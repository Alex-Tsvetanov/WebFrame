import { Device } from '../Device';
import { Locale } from './Locale';

export function getCurrentLocales(): Locale[] {
  const locales = Device.getDeviceLocales();
  if (locales.length) {
    return locales.map(Locale.parse);
  }

  if (typeof Intl !== 'undefined') {
    const intlLocale = Intl.DateTimeFormat().resolvedOptions().locale;
    return [Locale.parse(intlLocale)];
  }

  return [];
}

const LOCALE_PRIORITY_MULTIPLER = 100000;

const enum LocaleScore {
  MISMATCH = 0,
  FALLBACK,
  MATCH_LANGUAGE,
  MATCH_LANGUAGE_AND_REGION,
}

function isFallbackLocale(locale: Locale): boolean {
  return locale.language === 'en';
}

/**
 * Score is computed such that:
 * - When language matches, the "currentLocales" order defines the priority
 * - Language and region match will score higher than just language match only
 * for locales at the same index in the "currentLocales" array.
 * - Fallback language scores higher than mismatch
 * - Mismatch scores 0
 */
function computeLocaleScore(currentLocale: Locale, candidateLocale: Locale, currentLocalePriority: number): number {
  if (currentLocale.language === candidateLocale.language) {
    const premul = LOCALE_PRIORITY_MULTIPLER * currentLocalePriority;
    if (currentLocale.region === candidateLocale.region) {
      return premul + LocaleScore.MATCH_LANGUAGE_AND_REGION;
    } else {
      return premul + LocaleScore.MATCH_LANGUAGE;
    }
  }

  if (isFallbackLocale(candidateLocale)) {
    return LocaleScore.FALLBACK;
  }

  return LocaleScore.MISMATCH;
}

export function resolveLocale(currentLocales: readonly Locale[], candidateLocales: readonly Locale[]): number {
  let bestLocaleIndex: number = -1;
  let bestLocaleScore = LocaleScore.MISMATCH;
  let localeIndex = 0;

  for (const candidateLocale of candidateLocales) {
    if (currentLocales.length > 0) {
      let currentLocalePriority = currentLocales.length;
      for (const currentLocale of currentLocales) {
        const score = computeLocaleScore(currentLocale, candidateLocale, currentLocalePriority);
        if (score > bestLocaleScore) {
          bestLocaleIndex = localeIndex;
          bestLocaleScore = score;
        }
        currentLocalePriority--;
      }
    } else if (isFallbackLocale(candidateLocale)) {
      return localeIndex;
    }

    localeIndex++;
  }

  return bestLocaleIndex;
}
