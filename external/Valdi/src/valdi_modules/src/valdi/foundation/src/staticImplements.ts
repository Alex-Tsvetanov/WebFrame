/**
 * class decorator.
 *
 * Use this decorator to leverage the type system to guarantee that the class you're annotating
 * statically conforms to a given interface T.
 */
export function staticImplements<T>() {
  return <U extends T>(constructor: U) => {
    constructor;
  };
}
