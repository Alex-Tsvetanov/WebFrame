import type { ValdiProtobufModule } from '../src/ValdiProtobuf';

// This gets mapped correctly at runtime by package.json.tmpl
// And at build time by the tsconfig in this directory
import * as protobuf from '#root/src/valdi_protobuf/src/headless/protobuf-js';
import descriptorJson from '#root/src/valdi_protobuf/src/headless/descriptor';

// Normalize JSON shape (works for both ESM and CJS outputs)
const descriptor = (descriptorJson as any)?.default ?? descriptorJson;
const descriptorRoot = protobuf.Root.fromJSON(descriptor);
const FileDescriptorSet = descriptorRoot.lookupType("google.protobuf.FileDescriptorSet");

/** Webpack's require.context typing (used by Bazel/webpack builds) */
interface WebpackRequireContext {
  <T = any>(path: string): T;
  keys(): string[];
  resolve(path: string): string;
  id: string;
}

interface WebpackRequire {
  context(path: string, recursive: boolean, regExp: RegExp): WebpackRequireContext;
}
declare const require: WebpackRequire;


// Lazily obtain a webpack context, without ever touching bare `require` at top level
function getWebpackContext(): WebpackRequireContext | undefined {
  return require.context('../../../src', true, /\.protodecl\.js$/);
}

/**
 * Loads the compiled protodecl JS (bytes), decodes one or more concatenated
 * FileDescriptorSets, merges them, and returns the encoded union.
 *
 * @param path e.g. "proto.protodecl" (without ".js")
 */
function loadFn(path: string): Uint8Array {
  const context = getWebpackContext();
  if (!context) {
    throw new Error(
      "require.context is unavailable. Ensure your bundler provides it or swap in a custom loader."
    );
  }

  // NOTE: your regex is /\.protodecl\.js$/, so `path` should include `.protodecl`
  // ex: "proto.protodecl" -> "./proto.protodecl.js"
  const mod: any = context(`./${path}.js`);
  const bytes: Uint8Array = (mod?.default ?? mod) as Uint8Array;

  const reader = protobuf.Reader.create(bytes);
  const descriptorSets: Array<protobuf.Message<{}>> = [];

  while (reader.pos < reader.len) {
    try {
      const set = FileDescriptorSet.decode(reader);
      descriptorSets.push(set);
    } catch (e) {
      // eslint-disable-next-line no-console
      console.error("Decode failed at offset", reader.pos, e);
      break;
    }
  }

  const mergedFiles: any[] = [];
  for (let i = 0; i < descriptorSets.length; i++) {
    const f = (descriptorSets[i] as any).file as any[] | undefined;
    if (f && f.length) for (let j = 0; j < f.length; j++) mergedFiles.push(f[j]);
  }

  const mergedSet = FileDescriptorSet.create({ file: mergedFiles } as any);
  return FileDescriptorSet.encode(mergedSet).finish();
}

/* ---------- Headless module wiring ---------- */

// Minimal typing for the valdi dynamic require
declare function valdiRequire<T = any>(specifier: string): T;

const { HeadlessValdiProtobufModule } = valdiRequire<{
  HeadlessValdiProtobufModule: new (loader: (path: string) => Uint8Array) => ValdiProtobufModule;
}>("valdi_protobuf/src/headless/HeadlessValdiProtobufModule");

const moduleInstance: ValdiProtobufModule = new HeadlessValdiProtobufModule(loadFn);

/* Re-export as named functions (mirrors the original CommonJS export) */
export const createArena = moduleInstance.createArena.bind(moduleInstance);
export const loadMessages = moduleInstance.loadMessages.bind(moduleInstance);
export const parseAndLoadMessages = moduleInstance.parseAndLoadMessages?.bind(moduleInstance);
export const getFieldsForMessageDescriptor =
  moduleInstance.getFieldsForMessageDescriptor.bind(moduleInstance);
export const getNamespaceEntries = moduleInstance.getNamespaceEntries.bind(moduleInstance);
export const createMessage = moduleInstance.createMessage.bind(moduleInstance);
export const decodeMessage = moduleInstance.decodeMessage.bind(moduleInstance);
export const decodeMessageAsync = moduleInstance.decodeMessageAsync.bind(moduleInstance);
export const decodeMessageDebugJSONAsync =
  moduleInstance.decodeMessageDebugJSONAsync.bind(moduleInstance);
export const encodeMessage = moduleInstance.encodeMessage.bind(moduleInstance);
export const encodeMessageAsync = moduleInstance.encodeMessageAsync.bind(moduleInstance);
export const batchEncodeMessageAsync = moduleInstance.batchEncodeMessageAsync.bind(moduleInstance);
export const encodeMessageToJSON = moduleInstance.encodeMessageToJSON.bind(moduleInstance);
export const setMessageField = moduleInstance.setMessageField.bind(moduleInstance);
export const getMessageFields = moduleInstance.getMessageFields.bind(moduleInstance);
export const copyMessage = moduleInstance.copyMessage.bind(moduleInstance);

/* Optionally export the instance (handy for testing / DI) */
export const valdiProtobufModule: ValdiProtobufModule = moduleInstance;
