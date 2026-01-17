test(() => {
  const privateKey = Symbol();
  const publicKey = "public";
  class MyClass {
    [privateKey]() {
      return 42;
    }

    [publicKey]() {
      return 17;
    }
  }

  const instance = new MyClass();

  assertEquals(42, instance[privateKey]());
  assertEquals(17, instance["public"]());
}, module);
