test(() => {
  enum MyEnum {
    VALUE_0 = "Hello",
    VALUE_1 = 42,
  }

  assertEquals("Hello", MyEnum.VALUE_0);
  assertEquals(42, MyEnum.VALUE_1);

  const keys = Object.keys(MyEnum).sort();
  assertEquals(2, keys.length);
  assertEquals("VALUE_0", keys[0]);
  assertEquals("VALUE_1", keys[1]);

  const enum MyConstEnum {
    VALUE_0 = "World",
    VALUE_1 = 7,
  }

  assertEquals("World", MyConstEnum.VALUE_0);
  assertEquals(7, MyConstEnum.VALUE_1);
}, module);
