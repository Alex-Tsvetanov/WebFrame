import { BridgeError } from './BridgeError';
import { BridgeObserverEvent } from './BridgeObserverEvent';

/**
 * @ExportModel({
 *  ios: 'SCBridgeSubject',
 *  swift: 'BridgeSubject',
 *  android: 'com.snap.valdi.bridge_observables.BridgeSubject'
 * })
 */
export interface BridgeSubject<T> {
  onEvent: (
    type: BridgeObserverEvent,
    subscription: (() => void) | undefined,
    value: T | undefined,
    error: BridgeError | undefined,
  ) => void;

  subscribe: (
    onEvent: (
      type: BridgeObserverEvent,
      subscription: (() => void) | undefined,
      value: T | undefined,
      error: BridgeError | undefined,
    ) => void,
  ) => void;
}
