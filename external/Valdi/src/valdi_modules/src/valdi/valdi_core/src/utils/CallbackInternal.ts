import { AnyFunction, Callback, CALLBACK_KEY, OWNER_KEY } from './Callback';

/**
 * Assign an owner to the given function if it is a callback object, or merge
 * the callback objects if both the given value and previous value are
 * callback objects with the same owner. The owner is an opaque instance
 * that represents who emitted the callback. Only the owner can mutate
 * its callback object.
 *
 * This is a private function used by the framework and shouldn't be used
 * by consumers.
 *
 * @param owner
 * @param previousValue
 * @param newValue
 */
export function tryReuseCallback(
  owner: any,
  previousValue: any,
  newValue: ((...params: any[]) => void) | Callback<AnyFunction> | undefined,
): ((...params: any[]) => void) | undefined {
  if (!newValue) {
    return undefined;
  }

  const newCallback = newValue as Callback<AnyFunction>;
  if (!newCallback[CALLBACK_KEY]) {
    // Not a Callback object
    return newValue;
  }

  if (newCallback[OWNER_KEY]) {
    // The callback already has an owner assigned
    return newCallback;
  }

  if (previousValue instanceof Function) {
    const previousCallback = previousValue as Callback<AnyFunction>;
    if (previousCallback[OWNER_KEY] === owner) {
      // If owner match, we can update the callback directly
      previousCallback[CALLBACK_KEY] = newCallback[CALLBACK_KEY];
      return previousCallback;
    }
  }

  newCallback[OWNER_KEY] = owner;
  return newCallback;
}
