// @NativeInterface({marshallAsUntyped: true, ios: 'SCValdiViewFactory', iosImportPrefix: 'valdi_core', android: 'com.snap.valdi.ViewFactory'})
export interface ViewFactory {
  // this type tag only exists to enforce stronger TypeScript compiler guarantees
  // eslint-disable-next-line @typescript-eslint/naming-convention
  __tag: 'ViewFactory';
}
