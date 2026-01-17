/**
 * @ExportModel({ios: 'SCValdiFoundationError', android: 'com.snap.valdi.foundation.Error'})
 */
export interface Error {
  // contains the error message
  message: string;
  // contains the optional error code
  code?: number;
}
