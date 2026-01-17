import { Observable } from 'valdi_rxjs/src/Observable';
import { BridgeError } from '../types/BridgeError';
import { BridgeObservable } from '../types/BridgeObservable';
import { BridgeObserverEvent } from '../types/BridgeObserverEvent';

export function createBridgeSubscribeFn<T>(
  observable: Observable<T>,
): (
  onEvent: (
    type: BridgeObserverEvent,
    subscription: (() => void) | undefined,
    value: T | undefined,
    error: BridgeError | undefined,
  ) => void,
) => void {
  return (
    onEvent: (
      type: BridgeObserverEvent,
      subscription: (() => void) | undefined,
      value: T | undefined,
      error: BridgeError | undefined,
    ) => void,
  ) => {
    const subscription = observable.subscribe(
      (value: T) => {
        onEvent(BridgeObserverEvent.NEXT, undefined, value, undefined);
      },
      (error: BridgeError) => {
        onEvent(BridgeObserverEvent.ERROR, undefined, undefined, error);
      },
      () => {
        onEvent(BridgeObserverEvent.COMPLETE, undefined, undefined, undefined);
      },
    );

    onEvent(BridgeObserverEvent.RECEIVE_SUBSCRIPTION, () => subscription.unsubscribe(), undefined, undefined);
  };
}

export function convertObservableToBridgeObservable<T>(observable: Observable<T>): BridgeObservable<T> {
  return {
    subscribe: createBridgeSubscribeFn(observable),
  };
}
