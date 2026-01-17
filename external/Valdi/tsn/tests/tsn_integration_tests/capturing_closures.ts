test(() => {
  let value = 42;
  let cb = (input: number): number => value + input;

  assertEquals(50, cb(8));

  value = 40;
  assertEquals(48, cb(8));

  function recursiveFn(input: number, maxDepth: number): number {
    if (maxDepth === 0) {
      return input;
    }

    return recursiveFn(input * 2, maxDepth - 1);
  }

  assertEquals(48, recursiveFn(3, 4));

  let recursiveFn2 = (input: number, maxDepth: number): number => {
    if (maxDepth === 0) {
      return input;
    }

    return recursiveFn2(input * 2, maxDepth - 1);
  };

  assertEquals(96, recursiveFn2(3, 5));
}, module);
