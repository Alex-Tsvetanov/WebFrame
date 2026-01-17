/**
 * @ExportEnum({
 *  ios: 'SCBridgeObserverEvent',
 *  swift: 'BridgeObserverEvent',
 *  android: 'com.snap.valdi.bridge_observables.BridgeObserverEvent'
 * })
 */
export const enum BridgeObserverEvent {
  RECEIVE_SUBSCRIPTION,
  NEXT,
  ERROR,
  COMPLETE,
}
