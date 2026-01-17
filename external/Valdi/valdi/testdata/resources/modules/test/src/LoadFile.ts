import { getModuleFileEntryAsBytes, getModuleFileEntryAsString, loadModule } from 'valdi_core/src/Valdi';
import { arrayToString } from 'coreutils/src/Uint8ArrayUtils';

export function loadFromString(): any {
  const str = getModuleFileEntryAsString('test', 'src/test_json.json');
  return JSON.parse(str);
}

export function loadFromBytes(): any {
  const bytes = getModuleFileEntryAsBytes('test', 'src/test_json.json');
  const str = arrayToString(bytes);
  return JSON.parse(str);
}
