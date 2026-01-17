test(() => {
  let left = 14;
  let right = 7;

  assertEquals(14, left > right ? left : right);
  assertEquals(7, left < right ? left : right);
  assertEquals(14, left ? left : right);
  left = 0;
  assertEquals(7, left ? left : right);
}, module);
