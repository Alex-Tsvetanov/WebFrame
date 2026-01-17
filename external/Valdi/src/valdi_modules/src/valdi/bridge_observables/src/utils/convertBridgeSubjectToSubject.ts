import { AnonymousSubject, Subject } from 'valdi_rxjs/src/Subject';
import { BridgeSubject } from '../types/BridgeSubject';
import { convertBridgeObservableToObservable } from './convertBridgeObservableToObservable';
import { convertBridgeSubjectToObserver } from './convertBridgeSubjectToObserver';

export function convertBridgeSubjectToSubject<T>(subject: BridgeSubject<T>): Subject<T> {
  return new AnonymousSubject<T>(convertBridgeSubjectToObserver(subject), convertBridgeObservableToObservable(subject));
}
