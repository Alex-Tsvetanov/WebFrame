test(() => {
  function MyNumber(this: any, value: number) {
    this.value = value;
  }

  function MyOtherNumber() {}

  MyNumber.prototype.add = function (this: any, value: number) {
    this.value += value;
  };

  const nb = new (MyNumber as any)(42);

  assertEquals(42, nb.value);

  nb.add(1);

  assertEquals(43, nb.value);

  assertEquals(true, nb instanceof MyNumber);
  assertEquals(false, nb instanceof MyOtherNumber);
  assertEquals("object", typeof nb);
  assertEquals("number", typeof nb.value);

  // test prototypical inheritance
  function MyChildNumber(this: any) {
    MyNumber.call(this, 42);
  }
  MyChildNumber.prototype = new (MyNumber as any)();

  const child = new (MyChildNumber as any)();
  assertEquals(child.value, 42);
  child.add(1);
  assertEquals(child.value, 43);
}, module);
