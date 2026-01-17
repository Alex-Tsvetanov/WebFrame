test(async () => {
  // test async generator with for await
  async function* foo() {
    yield await Promise.resolve("a");
    yield await Promise.resolve("b");
    yield await Promise.resolve("c");
  }
  let str = "";
  for await (const val of foo()) {
    str = str + val;
  }
  assertEquals(str, "abc");

  // test for await with sync generator
  function* bar() {
    yield "a";
    yield "b";
    yield "c";
  }
  str = "";
  for await (const val of bar()) {
    str = str + val;
  }
  assertEquals(str, "abc");

  // test async generator with async execution
  function resolveLater(x: number): Promise<number> {
    return new Promise((resolve) => {
      setTimeout(() => {
        resolve(x);
      }, 10);
    });
  }
  async function* g1() {
    yield resolveLater(1);
    return 2;
  }
  const gen = g1();
  const res = await gen.next();
  assertEquals(res.value, 1);

  // test yield*
  let seq: number[] = [];
  function* y1() {
    yield 1;
    yield 2;
    yield 3;
  }
  async function* y2() {
    yield 10;
    yield* y1();
    yield 11;
  }
  for await (let i of y2()) {
    seq.push(i);
  }
  assertTrue([10, 1, 2, 3, 11].every((v, i) => v === seq[i]));
}, module);
