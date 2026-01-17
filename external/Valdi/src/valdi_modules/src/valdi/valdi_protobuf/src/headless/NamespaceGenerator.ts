// import { IArena, IMessage, JSONPrintOptions } from '../types';
import { Message as ValdiMessage } from '../Message';
import { IArena, IMessage, IMessageConstructor, IMessageNamespace, JSONPrintOptions } from '../../src/types';
import { FullyQualifiedName } from './FullyQualifiedName';
import { Message, INamespace, Root, Type, Writer, Reader, IConversionOptions } from './protobuf-js';
import { INativeMessageIndex } from '../../src/ValdiProtobuf';
import { ConsoleRepresentable } from 'valdi_core/src/ConsoleRepresentable';

export interface IProtobufJSMessageConstructor<T = {}> {
  new (): T;

  create<T extends Message<T>>(properties?: { [k: string]: any }): Message<T>;

  encode<T extends Message<T>>(message: T | { [k: string]: any }, writer?: Writer): Writer;

  encodeDelimited<T extends Message<T>>(message: T | { [k: string]: any }, writer?: Writer): Writer;

  decode<T extends Message<T>>(reader: Reader | Uint8Array): T;

  decodeDelimited<T extends Message<T>>(reader: Reader | Uint8Array): T;

  verify(message: { [k: string]: any }): string | null;

  fromObject<T extends Message<T>>(object: { [k: string]: any }): T;

  toObject<T extends Message<T>>(message: T, options?: IConversionOptions): { [k: string]: any };
}

export interface IProtobufJSNamespace {
  [key: string]: IProtobufJSMessageConstructor | IProtobufJSNamespace;
}

export interface IMonkeyPatchedProtobufJsNamespace {
  [key: string]: IMessageNamespace | IMonkeyPatchedProtobufJsNamespace;
}

export abstract class ProtobufJsMessageWrapper<MessageProps = {}> extends ValdiMessage<MessageProps> {
  private getPbMessage(): Message {
    return this as unknown as Message;
  }

  private getType(): Type {
    return this.getPbMessage().$type;
  }

  encode(): Uint8Array {
    return this.getType().encode(this).finish();
  }

  async encodeAsync(): Promise<Uint8Array> {
    return this.encode();
  }

  clone<T extends IMessage<MessageProps>>(this: T, arena?: IArena): T {
    return (this as unknown as ProtobufJsMessageWrapper).getType().create(this.toPlainObject() as any) as unknown as T;
  }

  toPlainObject(): MessageProps {
    return this.getType().toObject(this.getPbMessage()) as MessageProps;
  }

  toDebugJSON(options?: JSONPrintOptions): string {
    const plainObject = this.toPlainObject();
    if (options && (options & JSONPrintOptions.PRETTY) !== 0) {
      return JSON.stringify(plainObject, null, 2);
    } else {
      return JSON.stringify(plainObject);
    }
  }
}

// Make ProtobufJS Message inherits our ProtobufJsMessageWrapper.
Message.prototype = Object.create(ProtobufJsMessageWrapper.prototype, {
  constructor: {
    value: Message,
    writable: true,
    configurable: true,
  },
});
Object.setPrototypeOf(Message, ProtobufJsMessageWrapper);

export function generateNamespaceFromJSON(namespace: INamespace): IProtobufJSNamespace {
  const root = Root.fromJSON(namespace);
  return getNamespaceFromRoot(root);
}

export function getNamespaceFromRoot(root: Root): IProtobufJSNamespace {
  return root.nested as unknown as IProtobufJSNamespace;
}

function populateMonkeyPatchedNamespaceFromPbJsType(type: Type, namespace: IMessageNamespace): void {
  namespace.create = (arena: IArena | undefined, value?: any): IMessage => {
    return type.create(value) as unknown as IMessage;
  };
  namespace.decode = (arena: IArena | undefined, data: Uint8Array): IMessage => {
    return type.decode(data) as unknown as IMessage;
  };
  namespace.decodeAsync = (arena: IArena | undefined, data: Uint8Array): Promise<IMessage> => {
    return new Promise((resolve, reject) => {
      try {
        resolve(type.decode(data) as unknown as IMessage);
      } catch (err) {
        reject(err);
      }
    });
  };
  namespace.decodeDebugJSONAsync = (arena: IArena | undefined, json: string): Promise<IMessage> => {
    return new Promise((resolve, reject) => {
      try {
        resolve(type.fromObject(JSON.parse(json)) as unknown as IMessage);
      } catch (err) {
        reject(err);
      }
    });
  };
  namespace.encode = (value: IMessage | any): Uint8Array => {
    return type.encode(value).finish();
  };
  namespace.encodeAsync = (value: IMessage | any): Promise<Uint8Array> => {
    return new Promise((resolve, reject) => {
      try {
        resolve(type.encode(value).finish());
      } catch (err) {
        reject(err);
      }
    });
  };
}

function resolveTargetNamespace(
  current: IMonkeyPatchedProtobufJsNamespace,
  currentPackage: FullyQualifiedName
): IMonkeyPatchedProtobufJsNamespace {
  if (currentPackage.parent) {
    const parentNamespace = resolveTargetNamespace(current, currentPackage.parent);
    if (!parentNamespace[currentPackage.symbolName]) {
      parentNamespace[currentPackage.symbolName] = {};
    }
    return parentNamespace[currentPackage.symbolName] as IMonkeyPatchedProtobufJsNamespace;
  } else {
    if (!current[currentPackage.symbolName]) {
      current[currentPackage.symbolName] = {};
    }
    return current[currentPackage.symbolName] as IMonkeyPatchedProtobufJsNamespace;
  }
}

export function generateMonkeyPatchedNamespace(
  root: Root,
  types: readonly FullyQualifiedName[],
): IMonkeyPatchedProtobufJsNamespace {
  const out: IMonkeyPatchedProtobufJsNamespace = {};

  for (const type of types) {
    const pbJsType = root.lookupType(type.fullName);
    const namespace = resolveTargetNamespace(out, type);
    populateMonkeyPatchedNamespaceFromPbJsType(pbJsType, namespace as unknown as IMessageNamespace);
  }

  return out;
}

/*
function resolveTargetNamespace(current, currentPackage) {
  if (currentPackage.parent) {
    const parentNamespace = resolveTargetNamespace(current, currentPackage.parent);
    if (!parentNamespace[currentPackage.symbolName]) {
      parentNamespace[currentPackage.symbolName] = {};
    }
    return parentNamespace[currentPackage.symbolName];
  } else {
    if (!current[currentPackage.symbolName]) {
      current[currentPackage.symbolName] = {};
    }
    return current[currentPackage.symbolName];
  }
}
function generateMonkeyPatchedNamespace(root, types) {
    const out = {};
    for (const type of types) {
        const pbJsType = root.lookupType(type.fullName);
        const namespace = resolveTargetNamespace(out, type);
        populateMonkeyPatchedNamespaceFromPbJsType(pbJsType, namespace);
    }
    return out;
}
*/
