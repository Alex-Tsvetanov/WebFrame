interface State {
  tryCalled: boolean;
  catchCalled: boolean;
  finallyCalled: boolean;
  outerCalled: boolean;
}

test(() => {
  const cb = (state: State, throwInCatch: boolean) => {
    try {
      state.tryCalled = true;
      throw new Error("I am bad");
    } catch (err: unknown) {
      state.catchCalled = true;
      if (throwInCatch) {
        throw err;
      }
    } finally {
      state.finallyCalled = true;
    }
    state.outerCalled = true;
  };

  const state: State = {
    tryCalled: false,
    catchCalled: false,
    finallyCalled: false,
    outerCalled: false,
  };

  let didCatch = false;
  try {
    cb(state, false);
  } catch (err) {
    didCatch = true;
  }

  assertFalse(didCatch);

  assertTrue(state.tryCalled);
  assertTrue(state.catchCalled);
  assertTrue(state.finallyCalled);
  assertTrue(state.outerCalled);

  // Now try while throwing inside the catch. The finally should be called,
  // but not the outer

  didCatch = false;
  state.tryCalled = false;
  state.catchCalled = false;
  state.finallyCalled = false;
  state.outerCalled = false;

  try {
    cb(state, true);
  } catch (err) {
    didCatch = true;
  }

  assertTrue(didCatch);

  assertTrue(state.tryCalled);
  assertTrue(state.catchCalled);
  assertTrue(state.finallyCalled);
  assertFalse(state.outerCalled);
}, module);
