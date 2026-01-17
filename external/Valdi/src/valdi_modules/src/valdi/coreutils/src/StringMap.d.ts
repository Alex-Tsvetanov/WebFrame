// Deprecated: Use https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map

/**
 * @NativeClass({
 *   marshallAsUntypedMap: true
 * })
 */
export interface StringMap<T> {
  [key: string]: T | undefined;
}
