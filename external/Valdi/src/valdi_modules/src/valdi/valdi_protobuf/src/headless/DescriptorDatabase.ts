import { FieldType } from '../ValdiProtobuf';
import {
  DescriptorProto,
  EnumDescriptorProto,
  FieldDescriptorProto,
  FieldLabel,
  FileDescriptorProto,
  parseFileDescriptorSet,
} from './Descriptor.proto';
import { FullyQualifiedName } from './FullyQualifiedName';
import { IEnum, IField, IMapField, INamespace, IType, Root } from './protobuf-js';

export interface RegisteredDescriptor {
  name: FullyQualifiedName;
  isEnum: boolean;
}

interface MapType {
  keyType: string;
  valueType: string;
}

export class DescriptorDatabase {
  root = new Root();

  private allDescriptors: RegisteredDescriptor[] = [];
  private protobufJsNamespace: INamespace | undefined = undefined;
  private pendingTypesToResolve: IType[] = [];
  private mapTypeByName: Map<string, MapType> = new Map();

  constructor() {}

  resolve(): void {
    if (this.protobufJsNamespace) {
      for (const type of this.pendingTypesToResolve) {
        this.resolveMapFieldTypes(type);
      }
      this.pendingTypesToResolve = [];

      if (this.protobufJsNamespace.nested) {
        this.root.addJSON(this.protobufJsNamespace.nested);
      }
      this.protobufJsNamespace = undefined;
      this.root.resolveAll();
    }
  }

  private resolveMapFieldTypes(type: IType): void {
    for (const fieldName in type.fields) {
      const field = type.fields[fieldName]!;
      const mapType = this.mapTypeByName.get(field.type);
      if (mapType) {
        (field as IMapField).keyType = mapType.keyType;
        field.type = mapType.valueType;
      }
    }
  }

  getAllTypeNames(): FullyQualifiedName[] {
    return this.allDescriptors.map(d => d.name);
  }

  getAllMessageDescriptorTypeNames(): FullyQualifiedName[] {
    return this.allDescriptors.filter(d => !d.isEnum).map(d => d.name);
  }

  getAllDescriptors(): readonly RegisteredDescriptor[] {
    return this.allDescriptors;
  }

  private resolveNamespace(currentPackage: FullyQualifiedName): INamespace {
    const parts = currentPackage.fullName.split('.');

    if (!this.protobufJsNamespace) {
      this.protobufJsNamespace = {};
    }

    let namespace: INamespace = this.protobufJsNamespace;

    for (const part of parts) {
      if (!namespace.nested) {
        namespace.nested = {};
      }
      if (!namespace.nested[part]) {
        namespace.nested[part] = {};
      }

      namespace = namespace.nested[part] as INamespace;
    }

    return namespace;
  }

  addFileDescriptor(fileDescriptor: FileDescriptorProto): void {
    const packageName = fileDescriptor.package;

    const currentPackage = FullyQualifiedName.fromString(packageName);
    const namespace = this.resolveNamespace(currentPackage);

    if (fileDescriptor.messageType) {
      for (const messageType of fileDescriptor.messageType) {
        this.addDescriptor(namespace, currentPackage, messageType);
      }
    }

    if (fileDescriptor.enumType) {
      for (const enumType of fileDescriptor.enumType) {
        this.addEnumDescriptor(namespace, currentPackage, enumType);
      }
    }
  }

  private toTypeNameString(typeName: string | undefined): string {
    if (!typeName) {
      throw new Error('Expected type name');
    }

    if (typeName.startsWith('.')) {
      return typeName.substring(1);
    } else {
      return typeName;
    }
  }

  private resolveProtobufJsType(fieldDescriptorProto: FieldDescriptorProto): string {
    switch (fieldDescriptorProto.type) {
      case FieldType.DOUBLE:
        return 'double';
      case FieldType.FLOAT:
        return 'float';
      case FieldType.INT64:
        return 'int64';
      case FieldType.UINT64:
        return 'uint64';
      case FieldType.INT32:
        return 'int32';
      case FieldType.FIXED64:
        return 'fixed64';
      case FieldType.FIXED32:
        return 'fixed32';
      case FieldType.BOOL:
        return 'bool';
      case FieldType.STRING:
        return 'string';
      case FieldType.GROUP:
        throw new Error('Group are not supported');
      case FieldType.MESSAGE:
        return this.toTypeNameString(fieldDescriptorProto.typeName);
      case FieldType.BYTES:
        return 'bytes';
      case FieldType.UINT32:
        return 'uint32';
      case FieldType.ENUM:
        return this.toTypeNameString(fieldDescriptorProto.typeName);
      case FieldType.SFIXED32:
        return 'sfixed32';
      case FieldType.SFIXED64:
        return 'sfixed64';
      case FieldType.SINT32:
        return 'sint32';
      case FieldType.SINT64:
        return 'sint64';
    }
  }

  private toProtobufJsField(fieldDescriptorProto: FieldDescriptorProto): IField {
    let rule: string | undefined;
    const type = this.resolveProtobufJsType(fieldDescriptorProto);

    if (fieldDescriptorProto.label === FieldLabel.REPEATED) {
      rule = 'repeated';
    }

    return {
      rule,
      type,
      id: fieldDescriptorProto.number,
    };
  }

  private createMapType(descriptorProto: DescriptorProto): MapType {
    let keyType: string | undefined;
    let valueType: string | undefined;

    if (descriptorProto.field) {
      for (const field of descriptorProto.field) {
        if (field.name === 'key') {
          keyType = this.resolveProtobufJsType(field);
        } else if (field.name === 'value') {
          valueType = this.resolveProtobufJsType(field);
        }
      }
    }

    if (!keyType || !valueType) {
      throw new Error(`Could not resolve map type in: ${JSON.stringify(descriptorProto, null, 2)}`);
    }
    return { keyType, valueType };
  }

  private rebuildFullyQualifiedNameFromFull(fullName: string): FullyQualifiedName {
    const parts = fullName.split('.');
    let parent: FullyQualifiedName | undefined = undefined;
    let full = '';

    for (let i = 0; i < parts.length - 1; i++) {
      full = i === 0 ? parts[i] : `${full}.${parts[i]}`;
      parent = {
        symbolName: parts[i],
        fullName: full,
        parent: parent,
      };
    }

    return {
      symbolName: parts[parts.length - 1],
      fullName,
      parent,
    };
  }

  private addDescriptor(
    parentNamespace: INamespace,
    parentPackage: FullyQualifiedName,
    descriptorProto: DescriptorProto,
  ): void {
    const name = descriptorProto.name;
    const childPackage = new FullyQualifiedName(parentPackage, name);

    if (descriptorProto.options?.mapEntry) {
      const mapType = this.createMapType(descriptorProto);
      this.mapTypeByName.set(childPackage.fullName, mapType);
      return;
    }

    const type: IType = { fields: {} };

    if (descriptorProto.field) {
      for (const fieldDescriptor of descriptorProto.field) {
        type.fields[fieldDescriptor.name] = this.toProtobufJsField(fieldDescriptor);
      }
    }

    if (!parentNamespace.nested) {
      parentNamespace.nested = {};
    }

    parentNamespace.nested[name] = type;

    const fixedFQN = this.rebuildFullyQualifiedNameFromFull(childPackage.fullName);
    this.allDescriptors.push({ name: fixedFQN, isEnum: false });
    this.pendingTypesToResolve.push(type);

    if (descriptorProto.nestedType) {
      for (const nestedType of descriptorProto.nestedType) {
        this.addDescriptor(type, childPackage, nestedType);
      }
    }

    if (descriptorProto.enumType) {
      for (const nestedEnum of descriptorProto.enumType) {
        this.addEnumDescriptor(type, childPackage, nestedEnum);
      }
    }
  }

  private addEnumDescriptor(
    parentNamespace: INamespace,
    parentPackage: FullyQualifiedName | undefined,
    enumDescriptor: EnumDescriptorProto,
  ): void {
    const name = enumDescriptor.name;
    const values = enumDescriptor.value;

    const enumType: IEnum = {
      values: {},
    };

    const childPackage = new FullyQualifiedName(parentPackage, name);

    for (const value of values) {
      const enumValueName = value.name;
      const enumValueNumber = value.number;

      enumType.values[enumValueName] = enumValueNumber;
    }

    if (!parentNamespace.nested) {
      parentNamespace.nested = {};
    }
    parentNamespace.nested[name] = enumType;

    this.allDescriptors.push({ name: childPackage, isEnum: true });
  }

  getDescriptorSetFromBuffer(buffer: Uint8Array): Uint8Array {
    const tag = String.fromCharCode(...buffer.subarray(0, 8));
    if (tag != 'VALDIPRO') {
      // no tag, the entire buffer is a file descriptor set
      return buffer;
    }
    // found tag, skip the tag (8 bytes), index size (4 bytes), and the index
    const view = new DataView(buffer.buffer, buffer.byteOffset, buffer.byteLength);
    const indexSize = view.getUint32(8, true);
    return buffer.subarray(12 + indexSize);
  }

  addFileDescriptorSet(buffer: Uint8Array): void {
    const fileDescriptorSet = parseFileDescriptorSet(this.getDescriptorSetFromBuffer(buffer));

    if (fileDescriptorSet.file) {
      for (const file of fileDescriptorSet.file) {
        this.addFileDescriptor(file);
      }
    }
  }
}
