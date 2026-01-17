/**
 * Returns the value of `key` in `map`.
 * If `key` does not exist in the map, then set it to `defaultValue` and return it.
 */
import { StringMap } from 'coreutils/src/StringMap';

export function getOrSet<K, V>(key: K, map: Map<K, V>, defaultValue: V): V {
  const maybeValue = map.get(key);
  if (!maybeValue) {
    map.set(key, defaultValue);
    return defaultValue;
  }
  return maybeValue;
}

export function getOrLazySet<K, V>(key: K, map: Map<K, V>, defaultValueFn: () => V): V {
  const maybeValue = map.get(key);
  if (!maybeValue) {
    const newValue = defaultValueFn();
    map.set(key, newValue);
    return newValue;
  }
  return maybeValue;
}

export function fromStringMap<T>(strMap: StringMap<T>): Map<string, T> {
  const m = new Map<string, T>();
  for (const key of Object.keys(strMap)) {
    m.set(key, strMap[key]!);
  }
  return m;
}

/**
 * Constructs a Map for an array given a key provider
 */
export function arrayToMap<K, T>(arr: T[], key: (item: T) => K): Map<K, T> {
  const m = new Map<K, T>();
  for (const item of arr) {
    m.set(key(item), item);
  }
  return m;
}

/**
 * Provides a function that can be used to map arrays to string maps
 */
export function arrayToMapFn<K, T>(key: (item: T) => K): (arr: T[]) => Map<K, T> {
  return (arr: T[]) => arrayToMap(arr, key);
}

/**
 * Computes the union of two Maps and returns the result as a new Map
 */
export function unionMap<K, V>(m1: Map<K, V>, m2: Map<K, V>) {
  const result = new Map<K, V>();
  m1.forEach((v, k) => result.set(k, v));
  m2.forEach((v, k) => result.set(k, v));
  return result;
}
