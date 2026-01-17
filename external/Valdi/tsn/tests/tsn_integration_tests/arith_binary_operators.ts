test(() => {
  let value = 17;

  assertEquals(19, value + 2);
  assertEquals(15, value - 2);
  assertEquals(34, value * 2);
  assertEquals(8.5, value / 2);
  assertEquals(289, value ** 2);
  assertEquals(2, value % 15);
}, module);
