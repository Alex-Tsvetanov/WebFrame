test(() => {
  let value: number = 2;
  let result = 0;

  switch (value) {
    case 1:
      result |= 1;
      break;
    case 2:
      result |= 2;
      break;
    case 4:
      result |= 4;
      break;
    default:
      result = -1;
      break;
  }

  assertEquals(2, result);

  // Test with fallthrough
  result = 0;

  switch (value) {
    case 1:
      result |= 1;
      break;
    case 2:
      result |= 2;
    case 4:
      result |= 4;
      break;
    default:
      result = -1;
      break;
  }

  assertEquals(6, result);

  // Test default value

  result = 0;
  value = 9;

  switch (value) {
    case 1:
      result |= 1;
      break;
    case 2:
      result |= 2;
    case 4:
      result |= 4;
      break;
    default:
      result = -1;
      break;
  }

  assertEquals(-1, result);

  // test no default case
  let str: string = "abc";
  result = -1;
  switch (str) {
    case "cba":
      result = 0;
      break;
  }
  assertEquals(-1, result);

  // test nested switch
  let innerResult = 0;
  value = 1;
  result = 0;
  switch (value) {
    case 1:
      result |= 1;
      switch (value) {
        case 1:
          innerResult = 1;
          break;
        default:
          innerResult = -1;
      }
      break;
    case 2:
      result |= 2;
      break;
    case 4:
      result |= 4;
      break;
    default:
      result = -1;
      break;
  }
  assertEquals(1, result);
  assertEquals(1, innerResult);
}, module);
