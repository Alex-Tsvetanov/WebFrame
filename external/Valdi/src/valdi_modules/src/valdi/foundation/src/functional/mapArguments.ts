/**
 * Given a mapping function f(...x) => ...y, returns a method for transforming a function's signature
 * from f(...y) => returnType to f(...x) => returnType.
 *
 * @example
 * const concat = (x: string, y: string): string => x + y;
 * const concatNumbers = mapArguments((i: number, j: number): [string, string] => [i.toString(), j.toString()])(concat);
 * concatNumbers(123, 456); // returns "123456"
 *
 * @param mapper maps the desired input arguments to the required arguments
 * @returns a function that updates the arguments of the input function to the desired argument types
 */
export function mapArguments<ArgsIn extends unknown[], ArgsOut extends unknown[], ReturnType>(
  mapper: (...args: ArgsIn) => ArgsOut,
): (fn: (...args: ArgsOut) => ReturnType) => (...args: ArgsIn) => ReturnType {
  return fn =>
    (...args) =>
      fn(...mapper(...args));
}
