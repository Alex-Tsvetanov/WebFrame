type AsyncFunction<Args extends any[], Resolve> = (...args: Args) => Promise<Resolve>;

/**
 * Decorates an async function by internally caching returned Promises by a cache key
 * computed using {@link cacheKeyFn} with the arguments supplied. Only successfully resolved
 * Promises will be remain in cache. Any Promise that rejects is evicted.
 * @param cacheKeyFn generates a cache key using arguments supplied to the async function
 * @returns an async function that caches resolved values
 */
export function cacheResolved<Args extends any[], Resolve>(
  cacheKeyFn: (...args: Args) => string,
): (fn: AsyncFunction<Args, Resolve>) => AsyncFunction<Args, Resolve> {
  const cache: Record<string, Promise<Resolve>> = {};
  return (fn: AsyncFunction<Args, Resolve>) =>
    async (...args: Args) => {
      const key = cacheKeyFn(...args);
      const cached = cache[key];
      if (cached) {
        return cached;
      }
      const promise = fn(...args);
      cache[key] = promise;

      try {
        return await promise;
      } catch (e) {
        delete cache[key];
        throw e;
      }
    };
}
