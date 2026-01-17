import { getModuleLoader } from '../ModuleLoaderGlobal';

// We rely on a native number formatter to be set in Android to get around `toLocaleString()` being unavailable in quickjs.
let nativeNumberFormatter: ((value: number, fractionalDigits: number) => string) | undefined = undefined;

try {
  nativeNumberFormatter = getModuleLoader().load('NumberFormatting').formatNumber;
} catch (e: any) {
  // no-op; this module is only present on Android
}

// We rely on a native currency formatter to be set in Android to get around `Intl.NumberFormat` being unavailable in quickjs.
let nativeNumberWithCurrencyFormatter: ((value: number, currencyCode: string, minimumFractionDigits?: number, maximumFractionDigits?: number) => string) | undefined = undefined;

try {
  nativeNumberWithCurrencyFormatter = getModuleLoader().load('NumberFormatting').formatNumberWithCurrency;
} catch (e: any) {
  // no-op; this module is only present on Android
}

export function formatNumber(value: number, numFractionalDigits: number = -1): string {
  if (nativeNumberFormatter) {
    return nativeNumberFormatter(value, numFractionalDigits);
  }

  let options: any;
  if (numFractionalDigits !== -1) {
    options = {
      minimumFractionDigits: numFractionalDigits,
      maximumFractionDigits: numFractionalDigits,
    };
  }

  return value.toLocaleString(undefined, options);
}

export function formatNumberWithCurrency(
  value: number,
  currencyCode: string,
  options?: { minimumFractionDigits?: number; maximumFractionDigits?: number },
): string {
  if (nativeNumberWithCurrencyFormatter) {
    return nativeNumberWithCurrencyFormatter(value, currencyCode, options?.minimumFractionDigits, options?.maximumFractionDigits);
  }

  return new Intl.NumberFormat(undefined, {
    style: 'currency',
    currency: currencyCode,
    minimumFractionDigits: options?.minimumFractionDigits,
    maximumFractionDigits: options?.maximumFractionDigits,
  }).format(value);
}

/**
 * Convenience function to have javascript numbers converted to Long in native
 */
export function serializeLong(value: number | Long): Long {
  if (typeof value === 'number') {
    return Long.fromNumber(value);
  }
  return value;
}
