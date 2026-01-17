test(() => {
  let obj: any = {};

  assertEquals(undefined, obj.title);

  obj.title = "Hello World";

  assertEquals("Hello World", obj.title);

  obj = {
    title: "Goodbye",
  };

  assertEquals("Goodbye", obj.title);

  const title = "New title";

  obj = {
    title,
  };

  assertEquals("New title", obj.title);

  const newObject = {
    subobject: obj,
  };

  assertNotEquals(undefined, newObject.subobject);
  assertEquals("New title", newObject.subobject.title);
}, module);
