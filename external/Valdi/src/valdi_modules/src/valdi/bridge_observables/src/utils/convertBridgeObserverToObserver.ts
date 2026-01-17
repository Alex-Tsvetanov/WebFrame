import { Observer } from 'valdi_rxjs/src/types';
import { BridgeObserver } from '../types/BridgeObserver';
import { BridgeObserverEvent } from '../types/BridgeObserverEvent';

export function convertBridgeObserverToObserver<T>(observer: BridgeObserver<T>): Observer<T> {
  return {
    next: value => {
      observer.onEvent(BridgeObserverEvent.NEXT, undefined, value, undefined);
    },
    error: err => {
      observer.onEvent(BridgeObserverEvent.ERROR, undefined, undefined, err);
    },
    complete: () => {
      observer.onEvent(BridgeObserverEvent.COMPLETE, undefined, undefined, undefined);
    },
  };
}
