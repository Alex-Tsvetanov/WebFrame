/**
 * @NativeClass({ marshallAsUntyped: true, ios: 'SCValdiRef', iosImportPrefix: 'valdi_core', android: 'com.snap.valdi.utils.Ref'})
 * @deprecated use {@link NativeNode} instead
 */
export interface NativeView {
  // eslint-disable-next-line @typescript-eslint/naming-convention
  __nativeElementType?: 'nativeView';
}
