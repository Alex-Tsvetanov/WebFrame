import deepClone from 'foundation/src/deepClone';
import { deepEqual } from 'foundation/src/equality';

describe('deepClone', () => {
  for (const val of primitiveCases) {
    it(`should handle primitive types: ${val}`, () => {
      expect(deepClone(val)).toBe(val);
    });
  }

  for (const val of nonPrimitiveCases) {
    it(`should handle non primitive types: ${
      typeof val === 'object' ? val.constructor.name : typeof val
    } ${JSON.stringify(val)}`, () => {
      expect(deepClone(val)).not.toBe(val);
      expect(deepEqual(deepClone(val), val)).toEqual(true);
    });
  }

  it('should handle function', () => {
    const fn = (): void => {};
    expect(deepClone(fn)).toBe(fn);
  });

  it('should handle circular reference', () => {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const obj: any = {};
    // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-member-access
    obj.a = obj;
    expect(deepClone(obj)).not.toBe(obj);
    expect(deepClone(obj)).toEqual(obj);
  });
});

const primitiveCases = [0, 1, '', 'a', null, undefined, true, false] as const;

const nonPrimitiveCases = [
  new Date(),
  new RegExp('a'),
  new Map(),
  new Set(),
  new ArrayBuffer(1),
  Long.fromValue(1),
  {},
  { a: new Date(), b: [new Set([1, 2, 3]), 3, 'test', Long.fromValue(123)] },
];
