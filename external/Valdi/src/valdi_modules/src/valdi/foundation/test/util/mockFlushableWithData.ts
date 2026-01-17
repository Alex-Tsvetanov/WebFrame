import { mockFlushable } from './mockFlushable';

export function mockFlushableWithData<T>(data: T, waitForFlush?: boolean) {
  const flushable = mockFlushable(waitForFlush);
  return {
    handler: (completion: (data: T) => void) => {
      return flushable.handler(() => {
        completion(data);
      });
    },
    flush: flushable.flush,
  };
}
