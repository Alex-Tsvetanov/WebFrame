import { PropertyList } from 'valdi_tsx/src/PropertyList';
import { PersistentStoreNative } from '../src/PersistentStoreNative';

const enc = new TextEncoder();
const dec = new TextDecoder();

const microtask = (fn: () => void) => { void Promise.resolve().then(fn); };

type Entry = {
  value: ArrayBuffer | string;
  weight?: number;
  /** epoch seconds at which this entry expires (exclusive). undefined = no expiry */
  expiresAt?: number;
};

const storeMap = new Map<string, Entry>();
let nowOverrideSec: number | undefined;

const nowSec = () => (nowOverrideSec ?? Math.floor(Date.now() / 1000));
const cloneBuf = (b: ArrayBuffer) => b.slice(0);
const toBuf = (s: string) => enc.encode(s).buffer;
const toStr = (b: ArrayBuffer) => dec.decode(new Uint8Array(b));

const isExpired = (e: Entry) => e.expiresAt !== undefined && nowSec() >= e.expiresAt;
const sweepIfExpired = (k: string) => {
  const e = storeMap.get(k);
  if (e && isExpired(e)) storeMap.delete(k);
};

/* ---------- implementation ---------- */

const impl: PersistentStoreNative = {
  store(key, value, ttlSeconds, weight, completion) {
    microtask(() => {
      try {
        const entry: Entry = {
          value: typeof value === "string" ? value : cloneBuf(value),
          weight,
          expiresAt:
            ttlSeconds === undefined ? undefined :
            ttlSeconds <= 0 ? nowSec() : nowSec() + Math.floor(ttlSeconds),
        };
        storeMap.set(key, entry);
        completion(); // success
      } catch (e: any) {
        completion(String(e?.message ?? e ?? "store failed"));
      }
    });
  },

  fetch(key, completion, asString) {
    microtask(() => {
      sweepIfExpired(key);
      const e = storeMap.get(key);
      if (!e) {
        completion(undefined, "not found");
        return;
      }
      let v: ArrayBuffer | string = e.value;

      // honor asString flag by converting if needed
      if (asString && v instanceof ArrayBuffer) {
        v = toStr(v);
      } else if (!asString && typeof v === "string") {
        v = toBuf(v);
      }

      completion(v, undefined);
    });
  },

  fetchAll(completion) {
    microtask(() => {
      // collect keys first (no for..of, no Array.from)
      const keys: string[] = [];
      storeMap.forEach((_e, k) => keys.push(k));
      for (let i = 0; i < keys.length; i++) {
        sweepIfExpired(keys[i]);
      }

      const out: PropertyList = {};
      // safe iteration without for..of
      storeMap.forEach((e, k) => {
        out[k] = e.value;
      });
      completion(out);
    });
  },

  // inside setCurrentTime(...), replace the sweep loop with:
  setCurrentTime(timeSeconds) {
    nowOverrideSec = Math.floor(timeSeconds);
    const keys: string[] = [];
    storeMap.forEach((_e, k) => keys.push(k));
    for (let i = 0; i < keys.length; i++) {
      sweepIfExpired(keys[i]);
    }
  },

  exists(key, completion) {
    microtask(() => {
      sweepIfExpired(key);
      completion(storeMap.has(key));
    });
  },

  remove(key, completion) {
    microtask(() => {
      try {
        storeMap.delete(key);
        completion();
      } catch (e: any) {
        completion(String(e?.message ?? e ?? "remove failed"));
      }
    });
  },

  removeAll(completion) {
    microtask(() => {
      try {
        storeMap.clear();
        completion();
      } catch (e: any) {
        completion(String(e?.message ?? e ?? "removeAll failed"));
      }
    });
  },

};

/** Exported instance + setter so you can swap in a real native binding later. */
export let persistentStoreNative: PersistentStoreNative = impl;
export function setPersistentStoreNative(newImpl: PersistentStoreNative): void {
  persistentStoreNative = newImpl;
}