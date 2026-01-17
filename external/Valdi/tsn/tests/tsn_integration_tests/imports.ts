import { MY_CONST as HELLO_WORLD } from "./exports";

import * as ExportsModule from "./exports";

const exportsModulePromise = import("./exports");

test(() => {
  assertEquals(42, HELLO_WORLD);

  assertEquals(40, ExportsModule.MY_VALUE);

  assertTrue(exportsModulePromise instanceof Promise);
}, module);
