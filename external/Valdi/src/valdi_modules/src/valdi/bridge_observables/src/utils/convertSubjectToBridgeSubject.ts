import { Subject } from 'valdi_rxjs/src/Subject';
import { BridgeSubject } from '../types/BridgeSubject';
import { createBridgeSubscribeFn } from './convertObservableToBridgeObservable';
import { createBridgeOnEventFn } from './convertObserverToBridgeObserver';

export function convertSubjectToBridgeSubject<T>(subject: Subject<T>): BridgeSubject<T> {
  return {
    onEvent: createBridgeOnEventFn(subject),
    subscribe: createBridgeSubscribeFn(subject),
  };
}
