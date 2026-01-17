/**
 * Requires the value to be not be undefined, otherwise an error is thrown
 * @param value the value that should not be undefined
 * @param message an error message if undefined
 * @returns the defined value
 */
export function required<T>(value: T | undefined, message?: string): T {
  if (value === undefined) {
    throw new TypeError(message);
  }
  return value;
}
