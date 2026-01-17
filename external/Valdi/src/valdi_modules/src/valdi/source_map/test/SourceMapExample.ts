// Make sure to update SourceMap.spec.ts with the updated source maps definition every time
// this file changes.

interface SomeInterface {
  key: string;
  value: any;
}

export function doSomething(value: any): number {
  return value.left * value.right;
}
