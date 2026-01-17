/**
 * Represents a reference on the underlying runtime representation
 * for a Valdi element. A NativeNode can be used to manipulate
 * a node outside of TypeScript or to extract runtime information
 * about it, like it's current frame position.
 *
 * @NativeInterface({
 *   marshallAsUntyped: true,
 *   ios: 'SCValdiViewNodeProtocol',
 *   iosImportPrefix: 'valdi_core',
 *   android: 'com.snap.valdi.nodes.IValdiViewNode'
 * })
 */
export interface NativeNode {}
