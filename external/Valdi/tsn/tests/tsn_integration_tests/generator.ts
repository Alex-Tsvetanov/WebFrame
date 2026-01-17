test(() => {
  function* foo(a: number): any {
    const b = yield a + 1;
    let sum = b;
    // try a scoped variable
    {
      let a: number = -1;
      a = 1 + (yield a);
      sum += a;
    }
    const c = yield;
    sum += a + c;
    return sum;
  }

  const gen = foo(1);

  // {value: 2 (a+1), done: false}
  let res = gen.next();
  assertEquals(res.value, 2);
  assertEquals(res.done, false);

  // {value: -1 (scoped a), done: false}
  res = gen.next(2);
  assertEquals(res.value, -1);
  assertEquals(res.done, false);

  // {value: undefined, done: false}
  res = gen.next(3);
  assertEquals(res.value, undefined);
  assertEquals(res.done, false);

  // {value: 11 (2+(1+3)+(1+4)), done: true (return)}
  res = gen.next(4);
  assertEquals(res.value, 11);
  assertEquals(res.done, true);

  // {done: true, no more value}
  res = gen.next();
  assertEquals(res.value, undefined);
  assertEquals(res.done, true);

  // test exception from generator
  function* bar(): any {
    throw new Error("err");
  }
  const gen2 = bar();
  let exceptionMessage: string | undefined;
  try {
    gen2.next();
  } catch (err: any) {
    exceptionMessage = err.message;
  }
  assertEquals("err", exceptionMessage);

  // test generator method
  class GeneratorClass {
    private base: number;
    constructor(base: number) {
      this.base = base;
    }
    *generator(): IterableIterator<number> {
      yield this.base + 1;
      yield this.base + 2;
      return this.base + 3;
    }
  }
  const genobj = new GeneratorClass(3);
  const gen3 = genobj.generator();
  res = gen3.next();
  assertEquals(res.value, 4);
  assertEquals(res.done, false);
  res = gen3.next();
  assertEquals(res.value, 5);
  assertEquals(res.done, false);
  res = gen3.next();
  assertEquals(res.value, 6);
  assertEquals(res.done, true);

  // test yield*
  let seq: number[] = [];
  function* y1() {
    yield 1;
    yield 2;
    yield 3;
  }
  function* y2() {
    yield 10;
    yield* y1();
    yield 11;
  }
  for (let i of y2()) {
    seq.push(i);
  }
  assertTrue([10, 1, 2, 3, 11].every((v, i) => v === seq[i]));

  // test yield* with recursive generator
  seq = [];
  function* rec(n: number): Generator<number> {
    yield n;
    if (n < 3) {
      yield* rec(n + 1);
    }
  }
  for (let i of rec(1)) {
    seq.push(i);
  }
  assertTrue([1, 2, 3].every((v, i) => v === seq[i]));
}, module);
