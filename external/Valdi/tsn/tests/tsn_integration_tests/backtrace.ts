test(() => {
  function superNested() {
    throw new Error("This failed");
  }

  function nested() {
    superNested();
  }

  function outer() {
    nested();
  }

  let code = "function innerJS() { callback(); }";
  code += "function inJS() { innerJS(); }";
  code += " inJS();";
  const fn = new Function("callback", code);

  let stack = "";

  function another() {
    fn(outer);
  }
  try {
    another();
  } catch (err: any) {
    stack = err.stack;
  }

  assertTrue(stack.includes("superNested"));
  assertTrue(stack.includes("nested"));
  assertTrue(stack.includes("outer"));
  assertTrue(stack.includes("inJS"));
  assertTrue(stack.includes("innerJS"));
  assertTrue(stack.includes("another"));
}, module);
