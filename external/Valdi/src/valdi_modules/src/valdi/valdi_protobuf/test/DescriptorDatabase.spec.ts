import 'jasmine/src/jasmine';
import { IMapField } from 'valdi_protobuf/src/headless/protobuf-js';
import { getLoadedDescriptorDatabase } from './DescriptorDatabaseTestUtils';

describe('DescriptorDatabase', () => {
  it('can load type names', () => {
    const database = getLoadedDescriptorDatabase();

    expect(database.getAllTypeNames().map(t => t.fullName)).toEqual([
      'test3.Message3',
      'test.OtherMessage',
      'test.Message',
      'test.RepeatedMessage',
      'test.ParentMessage',
      'test.ParentMessage.ChildMessage',
      'test.ParentMessage.ChildEnum',
      'test.OneOfMessage',
      'test.OldMessage',
      'test.NewMessage',
      'test.OldEnumMessage',
      'test.OldEnumMessage.OldEnum',
      'test.NewEnumMessage',
      'test.NewEnumMessage.NewEnum',
      'test.MapMessage',
      'test.ExternalMessages',
      'test.Enum',
      'test2.Message2',
      'package_with_underscores.Message_With_Underscores',
      'package_with_underscores.M3ssage1WithNumb3r2',
      'package_with_underscores.Enum_With_Underscores',
    ]);

    const message = database.root.lookup('test.Message');
    expect(message).toBeTruthy();

    const nestedMessage = database.root.lookup('test.ParentMessage.ChildMessage');
    expect(nestedMessage).toBeTruthy();

    const underscorePackage = database.root.lookup('package_with_underscores.Message_With_Underscores');
    expect(underscorePackage).toBeTruthy();
  });

  it('can load root enum type', () => {
    const database = getLoadedDescriptorDatabase();
    const enumDescriptor = database.root.lookupEnum('test.Enum');
    expect(enumDescriptor.values['VALUE_0']).toBe(0);
    expect(enumDescriptor.values['VALUE_1']).toBe(1);
  });

  it('can load nested enum type', () => {
    const database = getLoadedDescriptorDatabase();
    const enumDescriptor = database.root.lookupEnum('test.ParentMessage.ChildEnum');
    expect(enumDescriptor.values['VALUE_0']).toBe(0);
    expect(enumDescriptor.values['VALUE_1']).toBe(1);
  });

  it('can load message', () => {
    const database = getLoadedDescriptorDatabase();
    const messageDescriptor = database.root.lookupType('test.Message');

    const int32Field = messageDescriptor.fields['int32'];
    expect(int32Field.id).toBe(1);
    expect(int32Field.type).toBe('int32');
    expect(int32Field.repeated).toBeFalsy();
    expect(int32Field.repeated).toBeFalsy();

    const otherMessageField = messageDescriptor.fields['otherMessage'];
    expect(otherMessageField.id).toBe(18);
    expect(otherMessageField.resolvedType).toBeTruthy();
    expect(otherMessageField.resolvedType?.fullName).toBe('.test.OtherMessage');
    expect(otherMessageField.repeated).toBeFalsy();
    expect(otherMessageField.repeated).toBeFalsy();
  });

  it('can load nested message', () => {
    const database = getLoadedDescriptorDatabase();
    const messageDescriptor = database.root.lookupType('test.ParentMessage.ChildMessage');

    const valueField = messageDescriptor.fields['value'];
    expect(valueField.id).toBe(1);
    expect(valueField.type).toBe('string');
    expect(valueField.repeated).toBeFalsy();
  });

  it('can load repeated message', () => {
    const database = getLoadedDescriptorDatabase();
    const messageDescriptor = database.root.lookupType('test.RepeatedMessage');

    const int32Field = messageDescriptor.fields['int32'];
    expect(int32Field.id).toBe(1);
    expect(int32Field.type).toBe('int32');
    expect(int32Field.repeated).toBeTruthy();
  });

  it('can load map message', () => {
    const database = getLoadedDescriptorDatabase();
    const messageDescriptor = database.root.lookupType('test.MapMessage');

    const stringToStringField = messageDescriptor.fields['stringToString'];
    expect(stringToStringField.id).toBe(1);
    expect(stringToStringField.map).toBeTrue();
    expect((stringToStringField as unknown as IMapField).keyType).toBe('string');
    expect(stringToStringField.type).toBe('string');

    const stringToDoubleField = messageDescriptor.fields['stringToDouble'];
    expect(stringToDoubleField.id).toBe(5);
    expect(stringToDoubleField.map).toBeTrue();
    expect((stringToDoubleField as unknown as IMapField).keyType).toBe('string');
    expect(stringToDoubleField.type).toBe('double');

    const longToStringField = messageDescriptor.fields['longToString'];
    expect(longToStringField.id).toBe(8);
    expect(longToStringField.map).toBeTrue();
    expect((longToStringField as unknown as IMapField).keyType).toBe('int64');
    expect(longToStringField.type).toBe('string');
  });
});
