// ---- Async helper
export function __tsn_async_helper<T>(gen: Iterator<Promise<any>, T, any>): Promise<T> {
  return new Promise<T>(function (resolve, reject) {
    const step = (func: typeof gen.next, arg?: any) => {
      try {
        const next = func.call(gen, arg);
        if (next.done) {
          // finished with success, resolve the promise
          return resolve(next.value);
        }
        // not finished, chain off the yielded promise and step again
        next.value.then(
          // keep stepping until next yield (original await) passing new value to yield
          v => step(gen.next, v),
          e => step(gen.throw!, e),
        );
      } catch (e) {
        // finished with failure, reject the promise
        return reject(e);
      }
    };
    // keep stepping until next yield (original await)
    step(gen.next);
  });
}

// ---- Async generator helper
interface AsyncGeneratorStep {
  await?: Promise<any>;
  yield?: any;
}

export function __tsn_async_generator_helper(gen: Iterator<AsyncGeneratorStep, any, any>): AsyncIterableIterator<any> {
  return {
    [Symbol.asyncIterator](): AsyncIterableIterator<any> {
      return this;
    },
    async next(a: any): Promise<IteratorResult<any, any>> {
      try {
        let res = gen.next(a);
        while (res.value?.await !== undefined) {
          res = gen.next(await res.value.await);
        }
        return Promise.resolve(res.done ? res.value : res.value.yield).then(v => ({ value: v, done: res.done }));
      } catch (e: any) {
        return Promise.reject(e);
      }
    },
    return(value?: any | PromiseLike<any>): Promise<IteratorResult<any, any>> {
      const res = gen.return?.(value);
      return res?.done ? Promise.resolve(res) : Promise.resolve({ done: true, value: undefined });
    },
    throw(e?: any): Promise<IteratorResult<any, any>> {
      const res = gen.throw?.(e);
      return res?.done ? Promise.resolve(res) : Promise.resolve({ done: true, value: undefined });
    },
  };
}

// ---- Generator to async generator adaptor
function asyncGeneratorAdaptor<T>(i: Iterator<T>): AsyncIterator<T> {
  return {
    next(): Promise<IteratorResult<T>> {
      return Promise.resolve(i.next());
    },
  };
}

export function __tsn_get_iterator<T>(gen: Iterable<T> | AsyncIterable<T>): Iterator<T> | AsyncIterator<T> {
  if (Symbol.iterator in (gen as Iterable<T>)) {
    return (gen as Iterable<T>)[Symbol.iterator].call(gen);
  } else {
    throw new Error('Object is not iterable');
  }
}

export function __tsn_get_async_iterator<T>(gen: Iterable<T> | AsyncIterable<T>): Iterator<T> | AsyncIterator<T> {
  if (Symbol.asyncIterator in (gen as Iterable<T>)) {
    return (gen as AsyncIterable<T>)[Symbol.asyncIterator].call(gen);
  } else if (Symbol.iterator in (gen as Iterable<T>)) {
    const i = (gen as Iterable<T>)[Symbol.iterator].call(gen);
    return asyncGeneratorAdaptor(i);
  } else {
    throw new Error('Object is not iterable');
  }
}
