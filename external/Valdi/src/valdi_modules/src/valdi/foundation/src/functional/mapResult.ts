type FunctionWithArgs<Args extends any[], Result> = (...args: Args) => Result;

/**
 * Given a mapping function f(x) => y, returns a method for transforming a function's signature
 * from f(...) => x to f(...) => y.
 *
 * @example
 * const multiply = (x: number, y: number): number => x * y;
 * const multiplyString = mapResult((x: number) => x.toString())(multiply);
 * multiplyString(4, 7); // returns "28"
 *
 * @param mapper maps the return value from the provided function to the desired output
 * @returns a function that updates the return value of the input function to the desired value
 */
export function mapResult<Args extends any[], ResultIn, ResultOut>(
  mapper: (result: ResultIn) => ResultOut,
): (fn: FunctionWithArgs<Args, ResultIn>) => FunctionWithArgs<Args, ResultOut> {
  return fn =>
    (...args: Args) =>
      mapper(fn(...args));
}
