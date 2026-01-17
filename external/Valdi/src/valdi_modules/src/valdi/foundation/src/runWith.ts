/**
 * Convenience method for invoking a function with the specified input. Useful when the input
 * is a computed type, so that the type of the function to run doesn't need to be explicitly defined
 * @param input the input to runnable
 * @param runnable the function to run
 * @returns the output of runnable(input)
 */
export function runWith<T, R>(input: T, runnable: (input: T) => R): R {
  return runnable(input);
}
