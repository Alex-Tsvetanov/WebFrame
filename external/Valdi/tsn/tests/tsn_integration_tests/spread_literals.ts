test(() => {
  let obj: any = { title: "A Nice Title" };
  let newObj = { ...obj, subtitle: "A Subtitle" };

  assertEquals("A Nice Title", newObj.title);
  assertEquals("A Subtitle", newObj.subtitle);

  let array: any[] = [42];
  let subArray = [...array, 1, 2];

  assertEquals(3, subArray.length);
  assertEquals(42, subArray[0]);
  assertEquals(1, subArray[1]);
  assertEquals(2, subArray[2]);

  // test non-array iterable objects
  let set: Set<number> = new Set();
  set.add(42);
  let subset: Set<number> = new Set([...set, 1, 2]);
  assertTrue(subset.has(42));
  assertTrue(subset.has(1));
  assertTrue(subset.has(2));

  // test undefined
  let ud: any = undefined;
  let spreadWithUndefined = { ...ud, a: 1 };
  assertEquals(spreadWithUndefined.a, 1);
}, module);
