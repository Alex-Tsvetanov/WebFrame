import { Observable } from './Observable';
import { EmptyError } from './util/EmptyError';
import { SafeSubscriber } from './Subscriber';
import { filter } from './operators/filter';
import { firstValueFrom } from './firstValueFrom';
import { timeout } from './operators/timeout';

export function firstValueWhere<T>(source: Observable<T>, predicate: (value: T) => boolean, timeoutMs?: number): Promise<T>;

/**
 * Converts an observable to a promise by subscribing to the observable,
 * and returning a promise that will resolve as soon as the first value
 * that satisfies the provided predicate arrives from the observable. 
 * The subscription will then be closed.
 *
 * The function filters the observable stream using the provided predicate
 * function and returns a promise that resolves with the first emitted value
 * that passes the predicate test. A timeout can be specified to prevent
 * the promise from hanging indefinitely.
 *
 * If the observable stream completes before any values that satisfy the 
 * predicate were emitted, the returned promise will reject with {@link EmptyError}.
 *
 * If the specified timeout is reached before a value that satisfies the
 * predicate is emitted, the returned promise will reject with a timeout error.
 *
 * If the observable stream emits an error, the returned promise will reject
 * with that error.
 *
 * **WARNING**: Only use this with observables you *know* will emit at least one value
 * that satisfies the predicate, *OR* complete, within the specified timeout period. 
 * If the source observable does not emit a value that passes the predicate test, 
 * complete, or timeout, you will end up with a promise that is hung up, and potentially 
 * all of the state of an async function hanging out in memory.
 *
 * ## Example
 *
 * Wait for the first odd number from a stream and emit it from a promise in
 * an async function
 *
 * ```ts
 * import { interval, firstValueWhere } from 'valdi_rxjs';
 *
 * async function execute() {
 *   const source$ = interval(1000); // emits 0, 1, 2, 3, 4, ...
 *   const firstOddNumber = await firstValueWhere(source$, x => x % 2 === 1, 5000);
 *   console.log(`The first odd number is ${ firstOddNumber }`);
 * }
 *
 * execute();
 *
 * // Expected output:
 * // 'The first odd number is 1'
 * ```
 *
 * @see {@link firstValueFrom}
 * @see {@link filter}
 * @see {@link timeout}
 *
 * @param source the observable to convert to a promise
 * @param predicate a function that takes a value and returns true if the value should be emitted
 * @param timeoutMs the timeout in milliseconds after which the promise will reject if no matching value is found
 */
export function firstValueWhere<T>(source: Observable<T>, predicate: (value: T) => boolean, timeoutMs?: number): Promise<T> {
  if (timeoutMs !== undefined) {
    return firstValueFrom(source.pipe(filter(predicate), timeout(timeoutMs)));
  }
  return firstValueFrom(source.pipe(filter(predicate)));
}
