import { BridgeError } from './BridgeError';
import { BridgeObserverEvent } from './BridgeObserverEvent';

/**
 * @ExportModel({
 *  ios: 'SCBridgeObservable',
 *  swift: 'BridgeObservable',
 *  android: 'com.snap.valdi.bridge_observables.BridgeObservable'
 * })
 */
export interface BridgeObservable<T> {
  subscribe: (
    onEvent: (
      type: BridgeObserverEvent,
      subscription: (() => void) | undefined,
      value: T | undefined,
      error: BridgeError | undefined,
    ) => void,
  ) => void;
}
