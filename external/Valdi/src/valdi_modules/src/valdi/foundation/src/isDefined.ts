/**
 * returns true is value is not undefined or null
 * for type narrowing
 *
 * ```ts
 * // 'a', 1
 * const array = ['a', null, 1, undefined].filter(isDefined)
 * ```
 */
export function isDefined<T>(value: T): value is NonNullable<T> {
  return value !== undefined && value !== null;
}
