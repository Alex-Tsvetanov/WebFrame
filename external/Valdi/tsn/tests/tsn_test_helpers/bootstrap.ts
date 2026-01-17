declare const global: any;

function assertEquals<T>(left: T, right: T): void {
  if (left !== right) {
    throw new Error(`Objects should be equal: ${left} vs ${right}`);
  }
}

function assertNotEquals<T>(left: T, right: T): void {
  if (left === right) {
    throw new Error(`Objects should NOT be equal: ${left} vs ${right}`);
  }
}

function assertTrue(value: boolean) {
  assertEquals(value, true);
}

function assertFalse(value: boolean) {
  assertEquals(value, false);
}

// Sanity check to verify that asserts work

function verifyAsserts(): boolean {
  try {
    assertEquals(42, 42);
    assertNotEquals(42, 43);
  } catch (err) {
    return false;
  }

  try {
    assertEquals(42, 43);
    assertNotEquals(42, 42);
    return false;
  } catch (err) {}

  return true;
}

if (!verifyAsserts()) {
  const errorMessage = "Asserts are broken!";
  console.error(errorMessage);
  throw new Error(errorMessage);
}

global.assertEquals = assertEquals;
global.assertNotEquals = assertNotEquals;
global.assertTrue = assertTrue;
global.assertFalse = assertFalse;

global.failedTests = [];
global.lastTestComplete = Promise.resolve();
global.test = (cb: () => TestFuncReturn, m: Module) => {
  global.lastTestComplete = global.lastTestComplete
    .then(() => console.log(`[ RUN      ] ${m.path}`))
    .then(cb)
    .then(
      () => console.log(`[  PASSED  ] ${m.path}`),
      (e: Error) => {
        console.log(`[  FAILED  ] ${m.path}`);
        console.error(e.message + e.stack);
        global.failedTests.push(m.path);
      },
    );
};
