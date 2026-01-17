test(() => {
  const left = 1;
  let right = 2;
  right = 1;

  const other = Number.parseInt("2");

  let ok = false;
  if (left === right && left !== other && right !== other) {
    ok = true;
  } else {
    ok = false;
  }
  assertTrue(ok);

  ok = false;
  if (left !== right && left === other && right === other) {
    ok = false;
  } else {
    ok = true;
  }

  assertTrue(ok);
}, module);
