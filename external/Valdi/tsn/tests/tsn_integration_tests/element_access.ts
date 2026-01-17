test(() => {
  const object = { array: [0, 1, 2] };

  assertEquals(0, object.array[0]);
  assertEquals(1, object.array[1]);
  assertEquals(2, object.array[2]);

  object.array[0]++;

  assertEquals(1, object.array[0]);

  let value = 7;
  let index = 1;

  object.array[index] = value;

  assertEquals(7, object.array[1]);
}, module);
