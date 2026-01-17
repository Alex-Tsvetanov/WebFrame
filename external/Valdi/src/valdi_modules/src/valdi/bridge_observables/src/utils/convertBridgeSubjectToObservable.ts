import { Observable } from 'valdi_rxjs/src/Observable';
import { BridgeSubject } from '../types/BridgeSubject';
import { convertBridgeObservableToObservable } from './convertBridgeObservableToObservable';

export function convertBridgeSubjectToObservable<T>(subject: BridgeSubject<T>): Observable<T> {
  return convertBridgeObservableToObservable(subject);
}
