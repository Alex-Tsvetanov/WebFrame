import type { JSONPath, ModificationOptions } from 'jsonc-parser';
import { applyEdits, modify, parse } from 'jsonc-parser';

export class JSONCFile<T> {
  constructor(
    private content: string,
    readonly value: T,
  ) {}

  static parse<T>(content: string): JSONCFile<T> {
    const parsed = parse(content) as T;

    return new JSONCFile(content, parsed);
  }

  static fromObject<T>(object: T): JSONCFile<T> {
    const content = JSON.stringify(object, null, 2);
    return JSONCFile.parse<T>(content);
  }

  toJSONString(): string {
    return this.content;
  }

  updating(path: JSONPath, value: unknown, options: ModificationOptions): JSONCFile<T> {
    const edits = modify(this.content, path, value, options);

    return new JSONCFile(applyEdits(this.content, edits), this.value);
  }
}
