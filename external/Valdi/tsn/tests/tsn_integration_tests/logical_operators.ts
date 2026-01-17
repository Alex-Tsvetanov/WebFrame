test(() => {
  let left = 14;
  let right = 7;

  assertEquals(7, left && right);
  assertEquals(14, left || right);

  assertEquals(false, !left);
  left = 0;
  assertEquals(true, !left);
}, module);
