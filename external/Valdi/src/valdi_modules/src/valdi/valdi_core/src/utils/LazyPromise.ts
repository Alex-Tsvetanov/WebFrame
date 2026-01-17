export interface ILazyPromise<T> {
  /**
   * Returns the cached result of the promise if available,
   * or undefined otherwise.
   */
  value: T | undefined;

  /**
   * Returns a promise that will resolve the underlying value.
   * This will trigger the load if the promise was not loaded yet.
   */
  promise: Promise<T>;

  then<F>(cb: (value: T) => F): ILazyPromise<F>;
}

class LazyPromise<T> implements ILazyPromise<T> {
  value: T | undefined = undefined;

  get promise(): Promise<T> {
    const value = this.value;
    if (value !== undefined) {
      return Promise.resolve(value);
    }

    return this.importFn!().then(v => {
      this.value = v;
      this.importFn = undefined;
      return v;
    });
  }

  private importFn: (() => Promise<T>) | undefined;

  constructor(importFn: () => Promise<T>) {
    this.importFn = importFn;
  }

  then<F>(cb: (value: T) => F): ILazyPromise<F> {
    return new LazyPromise<F>(() => this.promise.then(cb));
  }
}

/**
 * Create a Lazy Promise from a promise provider.
 * A lazy promise will only be executed when the underlying promise
 * is retrieved from the returned ILazyPromise instance.
 */
export function lazyPromise<T>(promiseFn: () => Promise<T>): ILazyPromise<T> {
  return new LazyPromise(promiseFn);
}
