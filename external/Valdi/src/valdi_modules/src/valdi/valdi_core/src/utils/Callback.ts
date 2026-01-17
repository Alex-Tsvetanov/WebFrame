export const OWNER_KEY = Symbol();
export const CALLBACK_KEY = Symbol();

export type AnyFunction = (...args: any[]) => any;

/**
 * A Callback is a function type that can be updated and
 * passed through view model properties or element attributes
 * without causing a component re-render or element update.
 * The callback instance stays stable across renders, only the
 * backing callable is updated.
 *
 * It is well suited to represent a callback on which the
 * component that receives it does not need to be re-rendered
 * whenever the backing callable changes, which is typical for
 * touch callbacks like `onTap` or `onDrag`.
 */
export interface Callback<F extends AnyFunction> {
  (...params: Parameters<F>): ReturnType<F>;
  [OWNER_KEY]: any;
  [CALLBACK_KEY]: F;
}

/**
 * Creates a reusable Callback object from the given arrow function.
 * When called, the returned Callback will call the given arrow function.
 * When passed as a view model property or element attribute, the Callback
 * object will be merged with the previous Callback object for the property
 * or attribute if there was one, preventing re-render or element update.
 *
 * @param callback The function that should be called whenever the returned Callback
 * object is called.
 */
export function createReusableCallback<F extends AnyFunction>(callback: F): Callback<F> {
  const fn: Callback<F> = function (this: any, ...args: Parameters<F>): ReturnType<F> {
    return fn[CALLBACK_KEY].apply(this, args);
  } as any;

  fn[CALLBACK_KEY] = callback;

  return fn;
}
