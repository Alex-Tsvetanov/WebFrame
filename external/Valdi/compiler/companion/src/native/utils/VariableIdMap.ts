import { NativeCompilerBuilderVariableID } from '../builder/INativeCompilerBuilder';

export class VariableIdMap<V extends Object | undefined> {
  get size(): number {
    return this.map.size;
  }

  private map = new Map<number, V>();

  has(key: NativeCompilerBuilderVariableID): boolean {
    return this.map.has(key.variable);
  }

  set(key: NativeCompilerBuilderVariableID, value: V): void {
    this.map.set(key.variable, value);
  }

  get(key: NativeCompilerBuilderVariableID): V | undefined {
    return this.map.get(key.variable);
  }

  delete(key: NativeCompilerBuilderVariableID): void {
    this.map.delete(key.variable);
  }

  toString(): string {
    let items: string[] = [];
    for (const [key, item] of this.map) {
      if (item) {
        items.push(item.toString());
      }
    }

    return items.toString();
  }
}
