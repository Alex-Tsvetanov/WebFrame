import { mockFlushable } from './mockFlushable';

export function mockFlushableWithDataAndError<T>(data: T | undefined, waitForFlush?: boolean) {
  const flushable = mockFlushable(waitForFlush);
  return {
    handler: (completion: (data?: T, error?: any) => void) => {
      return flushable.handler(() => {
        if (data === undefined) {
          completion(undefined, new Error('No Data'));
        }
        if (data === null) {
          completion(undefined, new Error('Invalid Data'));
        }
        completion(data);
      });
    },
    flush: flushable.flush,
  };
}
