test(() => {
  const obj: any = { obj: { title: "Hello World" } };

  let value = obj.obj?.title;

  assertEquals("Hello World", value);

  value = obj.obj2?.title;

  assertEquals(undefined, value);

  const fn = () => 42;

  const obj2: any = { callback: fn };

  value = obj2.callback?.();

  assertEquals(42, value);

  value = obj2.callback2?.();

  assertEquals(undefined, value);

  // test ??=
  const a: any = { duration: 50 };
  assertEquals((a.speed ??= 25), 25);
  assertEquals(a.speed, 25);
  assertEquals((a.duration ??= 10), 50);
  assertEquals(a.duration, 50);

  // test ||=
  const b = { duration: 50, title: "" };
  assertEquals((b.duration ||= 10), 50);
  assertEquals(b.duration, 50);
  assertEquals((b.title ||= "title is empty."), "title is empty.");
  assertEquals(b.title, "title is empty.");

  // test optional chaining short circuit
  let obj3: any = undefined;
  assertEquals(obj3?.obj.aaa, undefined); // ok .aaa is not evaluated
  // test optional element access short circuit
  assertEquals(obj3?.["obj"]["aaa"], undefined); // ok .aaa is not evaluated
  // test optional mixed short circuit
  assertEquals(obj3?.obj["aaa"], undefined); // ok .aaa is not evaluated

  // test optional chaining non-nil path
  let obj4: any = {};
  let err: any = undefined;
  try {
    // obj is undefined but it's not from `?.` so we will still evaluate .aaa
    console.log(obj4?.obj.aaa); // exception at .aaa
  } catch (e: any) {
    err = e;
  }
  assertTrue(err != undefined);
  // test optional element access
  err = undefined;
  try {
    console.log(obj4?.["obj"]["aaa"]); // exception at .aaa
  } catch (e: any) {
    err = e;
  }
  assertTrue(err != undefined);

  // test non-existing method call
  assertEquals(obj4.someNonExistentMethod?.(), undefined);

  // test delete expression
  const adventurer: any = {
    name: "Alice",
    cat: {
      name: "Dinah",
    },
  };
  delete adventurer?.name;
  assertEquals(adventurer.name, undefined);
  assertEquals(adventurer?.cat.name, "Dinah");
  delete adventurer?.cat?.name;
  assertEquals(adventurer.cat.name, undefined);
  delete adventurer?.cat?.rubbish;
  assertTrue(adventurer.cat !== undefined);

  // test function call chain
  function foo(): Array<number> | undefined {
    return undefined;
  }
  const ret = foo()
    ?.find((x) => x === 1)
    ?.toString();
  assertEquals(ret, undefined);
}, module);
