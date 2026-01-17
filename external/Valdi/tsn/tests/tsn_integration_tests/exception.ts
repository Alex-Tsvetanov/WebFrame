test(() => {
  let exceptionMessage: string | undefined;

  try {
    const cb = () => {
      throw new Error("This is an error");
    };
    cb();
  } catch (err: any) {
    exceptionMessage = err.message;
  }

  assertEquals("This is an error", exceptionMessage);
}, module);
