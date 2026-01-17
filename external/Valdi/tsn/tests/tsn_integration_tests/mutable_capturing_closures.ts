test(() => {
  let value = 1;

  let cb = (input: number): number => {
    const cur = input + value;
    value++;
    return cur;
  };

  assertEquals(1, cb(0));
  assertEquals(2, cb(0));
  assertEquals(13, cb(10));

  value -= 2;

  assertEquals(12, cb(10));
}, module);
