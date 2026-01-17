import { Observable } from 'valdi_rxjs/src/Observable';
import { BridgeSubject } from '../../types/BridgeSubject';
import { convertOptionalBridgeObservableToObservable } from './convertOptionalBridgeObservableToObservable';

export function convertOptionalBridgeSubjectToObservable<T>(
  subject?: BridgeSubject<T>,
  fallback?: Observable<T>,
): Observable<T> {
  return convertOptionalBridgeObservableToObservable(subject, fallback);
}
