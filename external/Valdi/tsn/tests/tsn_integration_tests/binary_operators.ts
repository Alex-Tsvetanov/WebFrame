test(() => {
  let value: any = 17;

  assertEquals(true, value == 17);
  assertEquals(true, value === 17);
  assertEquals(false, value != 17);
  assertEquals(false, value !== 17);
  assertEquals(false, value == null);
  assertEquals(false, value === null);
  assertEquals(false, value == undefined);
  assertEquals(false, value === undefined);

  value = "17";
  assertEquals(true, value == 17);
  assertEquals(false, value === 17);
  assertEquals(false, value != 17);
  assertEquals(true, value !== 17);
  assertEquals(false, value == null);
  assertEquals(false, value === null);
  assertEquals(false, value == undefined);
  assertEquals(false, value === undefined);

  value = null;
  assertEquals(false, value == 17);
  assertEquals(false, value === 17);
  assertEquals(true, value != 17);
  assertEquals(true, value !== 17);
  assertEquals(true, value == null);
  assertEquals(true, value === null);
  assertEquals(true, value == undefined);
  assertEquals(false, value === undefined);

  value = undefined;
  assertEquals(false, value == 17);
  assertEquals(false, value === 17);
  assertEquals(true, value != 17);
  assertEquals(true, value !== 17);
  assertEquals(true, value == null);
  assertEquals(false, value === null);
  assertEquals(true, value == undefined);
  assertEquals(true, value === undefined);
}, module);
