test(() => {
  let value = 17;

  // Postfix operators
  assertEquals(17, value++);
  assertEquals(18, value);

  assertEquals(18, value--);
  assertEquals(17, value);

  // Prefix operators
  assertEquals(18, ++value);
  assertEquals(18, value);
  assertEquals(17, --value);
  assertEquals(17, value);
  assertEquals(-17, -value);
  assertEquals(17, +value);

  const valueStr = "20";
  assertEquals(20, +valueStr);
}, module);
