test(() => {
  const MAX = 5;
  let i = 0;
  while (i < MAX) {
    i++;
  }
  assertEquals(5, i);

  for (let j = 0; j < MAX; j++) {
    i++;
  }

  assertEquals(10, i);

  while (true) {
    i++;
    break;
  }

  assertEquals(11, i);

  for (;;) {
    i++;
    break;
  }

  assertEquals(12, i);
}, module);
