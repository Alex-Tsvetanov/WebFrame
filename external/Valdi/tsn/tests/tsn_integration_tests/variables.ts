declare var test_global_var: number;
test_global_var = 1;

test(() => {
  let value: undefined | number;

  assertEquals(undefined, value);

  value = 1;

  assertEquals(1, value);

  let value2 = value;

  value = 2;

  assertEquals(2, value);
  assertEquals(1, value2);

  assertEquals(test_global_var, 1);
}, module);
