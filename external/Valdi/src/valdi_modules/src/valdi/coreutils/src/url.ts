import { StringMap } from 'coreutils/src/StringMap';

type EncodableParams = StringMap<string | number>;
type DecodedParams = StringMap<string>;

/**
 * String-encode a query param mapping.
 */
export function encodeQueryParams(params: EncodableParams): string {
  return Object.keys(params)
    .filter(k => params[k] !== undefined)
    .map(k => [k, params[k]!].map(encodeURIComponent).join('='))
    .join('&');
}

/**
 * Retrieve a map of from a querystring formatted string
 */
export function decodeQueryParams(paramString: string): DecodedParams {
  return paramString
    .split('&')
    .filter(pair => pair) // remove empty strings
    .map(pair => pair.split('=').map(decodeURIComponent))
    .reduce((acc: DecodedParams, pairParts: string[]) => {
      acc[pairParts[0]] = pairParts.length > 1 ? pairParts[1] : undefined;
      return acc;
    }, {});
}

/**
 * Generate a path string with query params.
 */
export function pathWithParams(path: string, params: EncodableParams): string {
  return [path, encodeQueryParams(params)].join('?');
}

/**
 * Parse key/value pairs from a URL's query string
 */
export function paramsFromUrl(url: string): DecodedParams {
  if (!url) return {};
  const parts = url.split('?');
  if (parts.length <= 1) return {};
  return decodeQueryParams(parts[1]);
}

/**
 * Validate if string matches url pattern and return results of the search
 */
function isUrl(url: string): RegExpMatchArray | null {
  return url.match(/^https?\:\/\/([^\/?#]+)(?:[\/?#]|$)/i);
}

/**
 * Parse hostname from URL
 */
export function hostnameFromUrl(url: string): string {
  const matches = isUrl(url);
  const host = matches && matches[1];
  return host || '';
}
