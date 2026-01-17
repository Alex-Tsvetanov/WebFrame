test(() => {
  const nonNull1 = "Hello";
  const nonNull2 = "World";
  const undefinedValue = undefined;
  const nullValue = null;

  let value = nonNull1 ?? undefinedValue;
  assertEquals("Hello", value);

  value = nonNull1 ?? nullValue;
  assertEquals("Hello", value);

  value = nonNull1 ?? nonNull2;
  assertEquals("Hello", value);

  value = undefinedValue ?? nonNull1;
  assertEquals("Hello", value);

  value = nullValue ?? nonNull1;
  assertEquals("Hello", value);

  value = nonNull2 ?? nonNull1;
  assertEquals("World", value);
}, module);
