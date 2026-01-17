import 'jasmine/src/jasmine';
import { Arena } from 'valdi_protobuf/src/Arena';
import { Message as ValdiMessage, Message } from 'valdi_protobuf/src/Message';
import {
  IProtobufJSMessageConstructor,
  generateMonkeyPatchedNamespace,
  getNamespaceFromRoot,
} from 'valdi_protobuf/src/headless/NamespaceGenerator';
import { Message as ProtobufJsMessage } from 'valdi_protobuf/src/headless/protobuf-js';
import { IArena, IMessageNamespace } from 'valdi_protobuf/src/types';
import { Base64 } from 'coreutils/src/Base64';
import { getLoadedDescriptorDatabase } from './DescriptorDatabaseTestUtils';
import { test } from './proto';

interface TypedMessageNamespace<I, C> {
  create: (arena: IArena, properties?: I | C) => C;
  decode: (arena: IArena, buffer: Uint8Array) => C;
}

function getTypedMessageNamespace<I, C>(getNamespace: (input: any) => any): TypedMessageNamespace<I, C> {
  const database = getLoadedDescriptorDatabase();
  const namespace = generateMonkeyPatchedNamespace(database.root, database.getAllMessageDescriptorTypeNames()) as any;

  const messageNamespace = getNamespace(namespace);
  if (!messageNamespace) {
    throw new Error('Could not resolve namespace');
  }
  return getNamespace(namespace) as unknown as TypedMessageNamespace<I, C>;
}

describe('NamespaceGenerator', () => {
  it('can generate protobuf js namespaces', () => {
    const database = getLoadedDescriptorDatabase();

    const namespace = getNamespaceFromRoot(database.root) as any;
    const Message = namespace.test?.Message as IProtobufJSMessageConstructor<test.IMessage>;

    expect(Message).toBeTruthy();
    expect(Message.create).toBeTruthy();
    expect(Message.encode).toBeTruthy();
    expect(Message.decode).toBeTruthy();

    const message = Message.create({
      int32: 42,
      string: 'Hello World',
    }) as test.IMessage;

    expect(message.int32).toEqual(42);
    expect(message.string).toEqual('Hello World');
  });

  it('ensures messages inherit both PbJS messages and Valdi messages', () => {
    const database = getLoadedDescriptorDatabase();

    const namespace = getNamespaceFromRoot(database.root) as any;
    const Message = namespace.test?.Message as IProtobufJSMessageConstructor<test.IMessage>;

    expect(Message).toBeTruthy();

    const message = Message.create({}) as test.IMessage;
    expect(message instanceof ProtobufJsMessage).toBeTrue();
    expect(message instanceof ValdiMessage).toBeTrue();
  });

  it('can generate monkey patched js namespaces', () => {
    const database = getLoadedDescriptorDatabase();
    const namespace = generateMonkeyPatchedNamespace(database.root, database.getAllMessageDescriptorTypeNames()) as any;
    const Message = namespace.test?.Message as IMessageNamespace;

    expect(Message).toBeTruthy();
    expect(Message.create).toBeTruthy();
    expect(Message.decodeAsync).toBeTruthy();
    expect(Message.encode).toBeTruthy();
    expect(Message.encodeAsync).toBeTruthy();
    expect(Message.decodeDebugJSONAsync).toBeTruthy();

    const message = Message.create(new Arena(), {
      int32: 42,
      string: 'Hello World',
    });

    expect((message as test.IMessage).int32).toEqual(42);
    expect((message as test.IMessage).string).toEqual('Hello World');

    const encoded = message.encode();
    expect(Base64.fromByteArray(encoded)).toBe('CCpyC0hlbGxvIFdvcmxk');

    const decodedMessage = Message.decode(message.$arena, encoded);
    expect((decodedMessage as test.IMessage).int32).toEqual(42);
    expect((decodedMessage as test.IMessage).string).toEqual('Hello World');
  });

  it('can encode and decode message fron monkey patched js namespace', () => {
    const Message = getTypedMessageNamespace<test.IMessage, test.Message>(namespace => namespace.test?.Message);

    const message = Message.create(new Arena(), {
      int32: 42,
      string: 'Hello World',
    });

    expect(message.int32).toEqual(42);
    expect(message.string).toEqual('Hello World');

    const encoded = message.encode();
    expect(Base64.fromByteArray(encoded)).toBe('CCpyC0hlbGxvIFdvcmxk');

    const decodedMessage = Message.decode(message.$arena, encoded);
    expect((decodedMessage as test.IMessage).int32).toEqual(42);
    expect((decodedMessage as test.IMessage).string).toEqual('Hello World');
  });

  it('supports nested messages in monkey patched js namespace', () => {
    const ParentMessage = getTypedMessageNamespace<test.IParentMessage, test.ParentMessage>(
      namespace => namespace.test?.ParentMessage,
    );

    const parentMessage = ParentMessage.create(new Arena(), {
      childMessage: {
        value: 'Hello World',
      },
      childEnum: test.ParentMessage.ChildEnum.VALUE_1,
    });

    expect(parentMessage.childEnum).toBe(test.ParentMessage.ChildEnum.VALUE_1);
    expect(parentMessage.childMessage?.value).toBe('Hello World');
  });

  // Does not yet work
  xit('supports nested map messages in monkey patched js namespace', () => {
    const MapMessage = getTypedMessageNamespace<test.IMapMessage, test.MapMessage>(
      namespace => namespace.test?.MapMessage,
    );

    const message = MapMessage.create(new Arena(), {
      stringToDouble: new Map([
        ['key1', 0],
        ['key2', 1],
      ]),
      stringToMessage: new Map([
        [
          'message',
          {
            value: 'Hello World',
          },
        ],
      ]),
    });

    expect(message.stringToDouble instanceof Map).toBeTrue();
    expect(message.stringToMessage instanceof Map).toBeTrue();

    expect(message.stringToDouble.get('key1')).toBe(0);
    expect(message.stringToDouble.get('key2')).toBe(1);

    const nestedMessage = message.stringToMessage.get('message');
    expect(nestedMessage).toBeDefined();
    expect(nestedMessage instanceof Message).toBeTruthy();
    expect(nestedMessage?.value).toBe('Hello World');

    const encoded = message.encode();
    expect(Base64.fromByteArray(encoded)).toBe('');
  });
});
