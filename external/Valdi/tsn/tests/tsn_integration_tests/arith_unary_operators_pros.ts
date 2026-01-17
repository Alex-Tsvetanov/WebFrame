test(() => {
  let obj = {
    inner: {
      value: 17,
    },
  };

  // Postfix operators
  assertEquals(17, obj.inner.value++);
  assertEquals(18, obj.inner.value);

  assertEquals(18, obj.inner.value--);
  assertEquals(17, obj.inner.value);

  // Prefix operators
  assertEquals(18, ++obj.inner.value);
  assertEquals(18, obj.inner.value);
  assertEquals(17, --obj.inner.value);
  assertEquals(17, obj.inner.value);
  assertEquals(-17, -obj.inner.value);
  assertEquals(17, +obj.inner.value);
}, module);
