test(() => {
  let value = 14;
  assertEquals(2, value & 3);
  assertEquals(15, value | 1);
  assertEquals(12, value ^ 2);
  assertEquals(-15, ~value);
  assertEquals(7, value >> 1);
  assertEquals(28, value << 1);

  value = -5;

  assertEquals(2147483645, value >>> 1);
}, module);
