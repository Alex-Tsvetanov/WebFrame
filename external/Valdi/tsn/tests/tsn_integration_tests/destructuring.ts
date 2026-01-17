function destructuring_test() {
  // Binding tests
  let array = [10, 20, 30, 40];
  {
    const [a, b] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
  }
  {
    const [a, , b] = array;
    assertEquals(a, 10);
    assertEquals(b, 30);
  }
  {
    let array: any = [1, { b: 2, c: 3 }];
    const [a, { b, c }] = array;
    assertEquals(a, 1);
    assertEquals(b, 2);
    assertEquals(c, 3);
  }
  {
    let array: any = [1, [2, 3]];
    const [a, [b, c]] = array;
    assertEquals(a, 1);
    assertEquals(b, 2);
    assertEquals(c, 3);
  }
  {
    const [a = 5, b] = [undefined, 20];
    assertEquals(a, 5);
    assertEquals(b, 20);
  }
  {
    const [a, b, ...rest] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertEquals(rest.length, 2);
    assertEquals(rest[0], 30);
    assertEquals(rest[1], 40);
  }
  {
    const [a, b, ...{ pop, push }] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertTrue(typeof pop === "function");
    assertTrue(typeof push === "function");
  }
  {
    const [a, b, ...[c, d]] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertEquals(c, 30);
    assertEquals(d, 40);
  }

  let obj = { a: 10, b: 20, c: 30, d: 40 };
  {
    const { a, b } = obj;
    assertEquals(a, 10);
    assertEquals(b, 20);
  }
  {
    const { a: a1, b: b1 } = obj;
    assertEquals(a1, 10);
    assertEquals(b1, 20);
  }
  {
    const { a: a1 = 5, b: b1 = 6 } = { a: undefined, b: undefined };
    assertEquals(a1, 5);
    assertEquals(b1, 6);
  }
  {
    const { a, b, ...rest } = obj;
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertEquals(rest.c, 30);
    assertEquals(rest.d, 40);
  }
  {
    const { a: a1, b: b1, ...rest } = obj;
    assertEquals(a1, 10);
    assertEquals(b1, 20);
    assertEquals(rest.c, 30);
    assertEquals(rest.d, 40);
  }
  {
    const obj = { a: 1, b: { c: 2, d: 3 } };
    const {
      a,
      b: { c: d, d: e },
    } = obj; // this binds a <- 1, d <-2, e <- 3
    assertEquals(a, 1);
    assertEquals(d, 2);
    assertEquals(e, 3);
  }
  {
    // TODO unsupported
    // const key = "b";
    // const { [key]: b } = obj;
    // assertEquals(b, 20);
  }

  // Assignment tests
  let a, b, a1, b1, c, d, rest, pop, push;
  {
    [a, b] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
  }
  {
    [a, , b] = array;
    assertEquals(a, 10);
    assertEquals(b, 30);
  }
  {
    let array: any = [1, { b: 2, c: 3 }];
    [a, { b, c }] = array;
    assertEquals(a, 1);
    assertEquals(b, 2);
    assertEquals(c, 3);
  }
  {
    let array: any = [1, [2, 3]];
    [a, [b, c]] = array;
    assertEquals(a, 1);
    assertEquals(b, 2);
    assertEquals(c, 3);
  }
  {
    [a = 5, b] = [undefined, 20];
    assertEquals(a, 5);
    assertEquals(b, 20);
  }
  {
    [a, b, ...rest] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertEquals(rest.length, 2);
    assertEquals(rest[0], 30);
    assertEquals(rest[1], 40);
  }
  {
    [a, b, ...{ pop, push }] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertTrue(typeof pop === "function");
    assertTrue(typeof push === "function");
  }
  {
    [a, b, ...[c, d]] = array;
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertEquals(c, 30);
    assertEquals(d, 40);
  }

  {
    ({ a, b } = obj);
    assertEquals(a, 10);
    assertEquals(b, 20);
  }
  {
    ({ a = 5, b = 6 } = { a: undefined, b: 7 });
    assertEquals(a, 5);
    assertEquals(b, 7);
  }
  {
    ({ a: a1, b: b1 } = obj);
    assertEquals(a1, 10);
    assertEquals(b1, 20);
  }
  {
    ({ a: a1 = 5, b: b1 = 6 } = { a: undefined, b: 7 });
    assertEquals(a1, 5);
    assertEquals(b1, 7);
  }
  {
    ({ a, b, ...rest } = obj);
    assertEquals(a, 10);
    assertEquals(b, 20);
    assertEquals(rest.c, 30);
    assertEquals(rest.d, 40);
  }
  {
    ({ a: a1, b: b1, ...rest } = obj);
    assertEquals(a1, 10);
    assertEquals(b1, 20);
    assertEquals(rest.c, 30);
    assertEquals(rest.d, 40);
  }
  {
    const obj = { a: 1, b: { c: 2, d: 3 } };
    let a, d, e;
    ({
      a,
      b: { c: d, d: e },
    } = obj); // this assigns a <- 1, d <-2, e <- 3
    assertEquals(a, 1);
    assertEquals(d, 2);
    assertEquals(e, 3);
  }
}
test(destructuring_test, module);
