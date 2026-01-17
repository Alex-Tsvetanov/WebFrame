export const MY_CONST = 42;
export function add(left: number, right: number): number {
  return left + right;
}

export enum MyEnum {
  VALUE = 1,
}

let MY_VALUE = 40;

export { MY_VALUE };

test(() => {
  assertEquals(module.exports.add, add);

  assertEquals(40, module.exports.MY_VALUE);
}, module);
