import { StringMap } from 'coreutils/src/StringMap';

export type StringInterner = (str: string) => number;

export class StringCache {
  private interner: StringInterner;
  private cache: StringMap<number>;

  constructor(interner: StringInterner) {
    this.interner = interner;
    this.cache = {};
  }

  get(str: string): number {
    let id = this.cache[str];
    if (id !== undefined) {
      return id;
    }

    id = this.interner(str);
    this.cache[str] = id;

    return id;
  }
}
