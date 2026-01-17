export function testMicrotask(setTimeoutCb: () => void, microtaskCb: () => void) {
    setTimeout(setTimeoutCb);
    queueMicrotask(microtaskCb);
}

export function testPromiseFulfillment(onResolved: () => void) {
    new Promise<void>((resolve) => {
        resolve();
    }).then(() => {
        onResolved();
    });
}
