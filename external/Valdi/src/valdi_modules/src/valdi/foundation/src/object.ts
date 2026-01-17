import { StringMap } from 'coreutils/src/StringMap';

export function stringifyKeys(map: StringMap<any>): StringMap<string> {
  return Object.keys(map).reduce<StringMap<any>>((smap, k) => {
    smap[k] = String(map[k]);
    return smap;
  }, {});
}

export function stringifyKeysMap(stringMap: StringMap<any>): Map<string, string> {
  const map = new Map();
  Object.keys(stringMap).forEach(k => {
    map.set(k, String(stringMap[k]));
  });
  return map;
}
