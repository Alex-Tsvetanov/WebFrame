// Mostly meant to check that retain and releases are inserted properly.

test(() => {
  let obj = {};

  for (let i = 0; i < 4; i++) {
    const newObj = {};
    if (i % 2 === 0) {
      obj = newObj;
    }
  }

  function MyClass(this: any) {}
  MyClass.prototype.link = function (this: any) {
    return this;
  };

  const test = new (MyClass as any)();
  test.link();
}, module);
