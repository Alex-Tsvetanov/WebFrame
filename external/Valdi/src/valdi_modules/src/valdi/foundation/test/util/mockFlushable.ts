export function mockFlushable(waitForFlush?: boolean) {
  let queueds: (() => void)[] = [];
  return {
    handler: (completion: () => void) => {
      if (waitForFlush) {
        queueds.push(completion);
      } else {
        completion();
      }
      return () => {};
    },
    flush: () => {
      for (const queued of queueds) {
        queued();
      }
      queueds = [];
    },
  };
}
