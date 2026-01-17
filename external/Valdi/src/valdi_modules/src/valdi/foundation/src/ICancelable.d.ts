/**
 * @ExportProxy({ios: 'SCValdiFoundationCancelable', android: 'com.snap.valdi.foundation.Cancelable'})
 */
export interface ICancelable {
  cancel(): void;
}
