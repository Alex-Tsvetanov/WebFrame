test(() => {
  const privateKey = Symbol();
  const publicKey = "public";
  class MyClass {
    get [privateKey]() {
      return 42;
    }

    get [publicKey]() {
      return 17;
    }
  }

  const instance = new MyClass();

  assertEquals(42, instance[privateKey]);
  assertEquals(17, instance["public"]);
}, module);
