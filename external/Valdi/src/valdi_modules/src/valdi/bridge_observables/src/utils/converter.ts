import { TypeConverter } from 'valdi_core/src/TypeConverter';
import { Observable } from 'valdi_rxjs/src/Observable';
import { Subject } from 'valdi_rxjs/src/Subject';
import { BridgeObservable } from '../types/BridgeObservable';
import { BridgeSubject } from '../types/BridgeSubject';
import { convertBridgeObservableToObservable } from './convertBridgeObservableToObservable';
import { convertBridgeSubjectToSubject } from './convertBridgeSubjectToSubject';
import { convertObservableToBridgeObservable } from './convertObservableToBridgeObservable';
import { convertSubjectToBridgeSubject } from './convertSubjectToBridgeSubject';

// @NativeTypeConverter
export function makeTypeConverter<T>(): TypeConverter<Observable<T>, BridgeObservable<T>> {
  return {
    toIntermediate: convertObservableToBridgeObservable,
    toTypeScript: convertBridgeObservableToObservable,
  };
}

// @NativeTypeConverter
export function makeSubjectTypeConverter<T>(): TypeConverter<Subject<T>, BridgeSubject<T>> {
  return {
    toIntermediate: convertSubjectToBridgeSubject,
    toTypeScript: convertBridgeSubjectToSubject,
  };
}
