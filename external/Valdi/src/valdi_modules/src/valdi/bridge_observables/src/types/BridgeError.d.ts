/**
 * @ExportModel({
 *  ios: 'SCBridgeError',
 *  swift: 'BridgeError',
 *  android: 'com.snap.valdi.bridge_observables.BridgeError'
 * })
 */
export interface BridgeError {
  message: string;
  stack?: string;
}
