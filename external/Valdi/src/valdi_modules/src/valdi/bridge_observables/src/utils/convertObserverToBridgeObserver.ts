import { Observer } from 'valdi_rxjs/src/types';
import { BridgeError } from '../types/BridgeError';
import { BridgeObserver } from '../types/BridgeObserver';
import { BridgeObserverEvent } from '../types/BridgeObserverEvent';

export function createBridgeOnEventFn<T>(
  observer: Observer<T>,
): (
  type: BridgeObserverEvent,
  subscription: (() => void) | undefined,
  value: T | undefined,
  error: BridgeError | undefined,
) => void {
  return (type, subscription, value, error) => {
    switch (type) {
      case BridgeObserverEvent.RECEIVE_SUBSCRIPTION:
        // nothing to do
        break;
      case BridgeObserverEvent.NEXT:
        observer.next(value!);
        break;
      case BridgeObserverEvent.ERROR:
        observer.error(error);
        break;
      case BridgeObserverEvent.COMPLETE:
        observer.complete();
        break;
    }
  };
}

export function convertObserverToBridgeObserver<T>(observer: Observer<T>): BridgeObserver<T> {
  return {
    onEvent: createBridgeOnEventFn(observer),
  };
}
