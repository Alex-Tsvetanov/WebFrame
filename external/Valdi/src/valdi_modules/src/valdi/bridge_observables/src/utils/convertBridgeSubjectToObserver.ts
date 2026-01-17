import { Observer } from 'valdi_rxjs/src/types';
import { BridgeSubject } from '../types/BridgeSubject';
import { convertBridgeObserverToObserver } from './convertBridgeObserverToObserver';

export function convertBridgeSubjectToObserver<T>(subject: BridgeSubject<T>): Observer<T> {
  return convertBridgeObserverToObserver(subject);
}
