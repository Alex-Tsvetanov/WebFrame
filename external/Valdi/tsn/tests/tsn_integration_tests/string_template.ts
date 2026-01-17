test(() => {
  let value = `Hello World`;
  assertEquals("Hello World", value);

  const name = "Ramzy";
  const suffix = "Goodbye";
  value = `Hello ${name} and ${suffix}!`;

  assertEquals("Hello Ramzy and Goodbye!", value);
}, module);
