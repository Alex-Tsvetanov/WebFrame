test(() => {
  const items = ["Hello", "World", "!"];
  let out = "";
  for (const key in items) {
    out += items[key];
  }
  assertEquals("HelloWorld!", out);
}, module);
