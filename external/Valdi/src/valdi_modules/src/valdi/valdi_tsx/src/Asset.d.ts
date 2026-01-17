/**
 * @NativeClass({
 *   marshallAsUntyped: true,
 *   ios: 'SCNValdiCoreAsset', iosImportPrefix: 'valdi_core',
 *   android: 'com.snapchat.client.valdi_core.Asset'
 * })
 */
export interface Asset {
  readonly path: string;
  readonly width: number;
  readonly height: number;
}
