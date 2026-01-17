test(() => {
  const items = ["Hello", "World", "!"];
  let out = "";
  for (const item of items) {
    out += item;
  }
  assertEquals("HelloWorld!", out);
}, module);
