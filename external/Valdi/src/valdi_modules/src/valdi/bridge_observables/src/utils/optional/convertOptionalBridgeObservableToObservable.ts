import { Observable } from 'valdi_rxjs/src/Observable';
import { EMPTY } from 'valdi_rxjs/src/observable/empty';
import { BridgeObservable } from '../../types/BridgeObservable';
import { convertBridgeObservableToObservable } from '../convertBridgeObservableToObservable';

export function convertOptionalBridgeObservableToObservable<T>(
  observable?: BridgeObservable<T>,
  fallback?: Observable<T>,
): Observable<T> {
  if (!observable) {
    return fallback ?? EMPTY;
  }
  return convertBridgeObservableToObservable(observable);
}
