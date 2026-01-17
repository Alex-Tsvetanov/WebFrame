type KeyedTransforms<T> = { [k in keyof T]?: (input: T[k]) => unknown };

/**
 * Given a type of object T and a set of desired transformations keyed by properties of T,
 * returns a function that applies those transformations to the respective properties of objects of type T
 * @example
 * interface Foo {
 *   count: number;
 *   message: string;
 * }
 * const squareCount = transformProperties<Foo>({
 *   count: (x): number => x * x,
 * });
 *
 * squareCount({ count: 2, message: 'foo' }); // returns { count: 4, message: 'foo' }
 * @param transforms describes how each property should be transformed
 */
export function transformProperties<T extends {}, Transforms extends KeyedTransforms<T> = KeyedTransforms<T>>(
  transforms: Transforms,
): (input: T) => { [k in keyof T]: Transforms[k] extends (input: T[k]) => infer Out ? Out : T[k] };

export function transformProperties<T extends {}>(
  transforms: KeyedTransforms<T>,
): (input: T) => Record<keyof T, unknown> {
  return (input: T) => {
    const transformed: Record<keyof T, unknown> = { ...input };
    for (const k in input) {
      const transform = transforms[k];
      if (transform) {
        transformed[k] = transform(input[k]);
      }
    }
    return transformed;
  };
}
