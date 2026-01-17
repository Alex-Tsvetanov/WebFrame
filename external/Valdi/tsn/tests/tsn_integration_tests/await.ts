function resolveLater(x: number): Promise<number> {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(x);
    }, 10);
  });
}

function resolveExceptionLater(x: number): Promise<number> {
  return new Promise((_, reject) => {
    setTimeout(() => {
      reject(new Error("errors"));
    }, 10);
  });
}

async function f1(x: number): Promise<number> {
  const y = await resolveLater(x);
  return y;
}

async function f2(x: number): Promise<any> {
  try {
    const y = await resolveExceptionLater(x);
    return y;
  } catch (e: any) {
    return e;
  }
}

async function f3(x: number): Promise<number> {
  let y = 0;
  for (let i of [1, 2, 3]) {
    y = await resolveLater(x);
  }
  return y;
}

async function rec(n: number): Promise<number[]> {
  if (n < 3) {
    return (await rec(n + 1)).concat([n]);
  } else {
    return Promise.resolve([n]);
  }
}

const f4 = async (x: number) => await resolveLater(x);

test(async () => {
  // test happy path
  const x = 10;
  const y = await f1(x);
  assertEquals(x, y);

  // test exception path
  const y1 = await f2(x);
  assertEquals(y1.message, "errors");

  // test await in for/of loop
  const y2 = await f3(x);
  assertEquals(x, y2);

  // test recursive async function
  const y3 = await rec(1);
  assertTrue([3, 2, 1].every((v, i) => v === y3[i]));

  // test arrow expression
  const y4 = await f4(x);
  assertEquals(x, y);
}, module);
