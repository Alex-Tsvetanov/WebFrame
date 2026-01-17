import { getModuleFileEntryAsBytes } from 'valdi_core/src/Valdi';
import { DescriptorDatabase } from 'valdi_protobuf/src/headless/DescriptorDatabase';

export function getLoadedDescriptorDatabase(): DescriptorDatabase {
  const protoDeclContent = getModuleFileEntryAsBytes('valdi_protobuf', 'test/proto.protodecl');
  const database = new DescriptorDatabase();
  database.addFileDescriptorSet(protoDeclContent);
  database.resolve();
  return database;
}
