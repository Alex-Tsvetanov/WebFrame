import { FieldType } from '../ValdiProtobuf';
import { IProtobufJSMessageConstructor, generateNamespaceFromJSON } from './NamespaceGenerator';

export const enum FieldLabel {
  OPTIONAL = 1,
  REQUIRED = 2,
  REPEATED = 3,
}

export interface FieldDescriptorProto {
  name: string;
  number: number;
  label: FieldLabel;
  type: FieldType;
  typeName?: string;
  oneofIndex?: number;
  jsonName?: string;
}

export interface DescriptorProto {
  name: string;
  field?: FieldDescriptorProto[];
  nestedType?: DescriptorProto[];
  enumType?: EnumDescriptorProto[];
  oneofDecl?: OneofDescriptorProto[];
  options?: MessageOptions;
}

export interface EnumValueDescriptorProto {
  name: string;
  number: number;
}

export interface EnumDescriptorProto {
  name: string;
  value: EnumValueDescriptorProto[];
}

export interface OneofDescriptorProto {
  name: string;
}

export interface MessageOptions {
  mapEntry: boolean;
}

export interface FileDescriptorProto {
  name: string;
  package: string;
  messageType?: DescriptorProto[];
  enumType?: EnumDescriptorProto[];
}

export interface FileDescriptorSet {
  file?: FileDescriptorProto[];
}

const descriptorNamespace = generateNamespaceFromJSON({
  nested: {
    FileDescriptorSet: {
      fields: {
        file: {
          type: 'FileDescriptorProto',
          id: 1,
          rule: 'repeated',
        },
      },
    },
    FileDescriptorProto: {
      fields: {
        name: {
          type: 'string',
          id: 1,
        },
        package: {
          type: 'string',
          id: 2,
        },
        messageType: {
          rule: 'repeated',
          type: 'DescriptorProto',
          id: 4,
        },
        enumType: {
          rule: 'repeated',
          type: 'EnumDescriptorProto',
          id: 5,
        },
      },
    },
    DescriptorProto: {
      fields: {
        name: {
          type: 'string',
          id: 1,
        },
        field: {
          rule: 'repeated',
          type: 'FieldDescriptorProto',
          id: 2,
        },
        nestedType: {
          rule: 'repeated',
          type: 'DescriptorProto',
          id: 3,
        },
        enumType: {
          rule: 'repeated',
          type: 'EnumDescriptorProto',
          id: 4,
        },
        options: {
          type: 'MessageOptions',
          id: 7,
        },
        oneofDecl: {
          rule: 'repeated',
          type: 'OneofDescriptorProto',
          id: 8,
        },
      },
    },

    FieldDescriptorProto: {
      fields: {
        name: {
          type: 'string',
          id: 1,
        },
        number: {
          type: 'int32',
          id: 3,
        },
        label: {
          type: 'Label',
          id: 4,
        },
        type: {
          type: 'Type',
          id: 5,
        },
        typeName: {
          type: 'string',
          id: 6,
        },
        defaultValue: {
          type: 'string',
          id: 7,
        },
        oneofIndex: {
          type: 'int32',
          id: 9,
        },
        jsonName: {
          type: 'string',
          id: 10,
        },
      },
      nested: {
        Type: {
          values: {
            TYPE_DOUBLE: 1,
            TYPE_FLOAT: 2,
            TYPE_INT64: 3,
            TYPE_UINT64: 4,
            TYPE_INT32: 5,
            TYPE_FIXED64: 6,
            TYPE_FIXED32: 7,
            TYPE_BOOL: 8,
            TYPE_STRING: 9,
            TYPE_GROUP: 10,
            TYPE_MESSAGE: 11,
            TYPE_BYTES: 12,
            TYPE_UINT32: 13,
            TYPE_ENUM: 14,
            TYPE_SFIXED32: 15,
            TYPE_SFIXED64: 16,
            TYPE_SINT32: 17,
            TYPE_SINT64: 18,
          },
        },
        Label: {
          values: {
            LABEL_OPTIONAL: 1,
            LABEL_REQUIRED: 2,
            LABEL_REPEATED: 3,
          },
        },
      },
    },
    OneofDescriptorProto: {
      fields: {
        name: {
          type: 'string',
          id: 1,
        },
      },
    },
    EnumDescriptorProto: {
      fields: {
        name: {
          type: 'string',
          id: 1,
        },
        value: {
          rule: 'repeated',
          type: 'EnumValueDescriptorProto',
          id: 2,
        },
      },
    },
    EnumValueDescriptorProto: {
      fields: {
        name: {
          type: 'string',
          id: 1,
        },
        number: {
          type: 'int32',
          id: 2,
        },
      },
    },
    MessageOptions: {
      fields: {
        mapEntry: {
          type: 'bool',
          id: 7,
        },
      },
      extensions: [[1000, 536870911]],
      reserved: [[8, 8]],
    },
  },
});

export function parseFileDescriptorSet(data: Uint8Array): FileDescriptorSet {
  return (descriptorNamespace.FileDescriptorSet as IProtobufJSMessageConstructor).decode(data) as FileDescriptorSet;
}
