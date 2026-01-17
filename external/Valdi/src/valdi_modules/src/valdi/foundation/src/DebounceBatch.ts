import { Debounce } from './Debounce';

interface Item<T, R> {
  value: T;
  completion: (value?: R, error?: any) => void;
}

/**
 * Allow the batching of multiple async operations
 * by immediately scheduling them
 * and automatically running them all at the same time
 * trough a single logic call after some debounced delay
 */
export class DebounceBatch<T, R> {
  private debouncer: Debounce<Item<T, R>>;

  /**
   * Create a DebounceBatch object
   * @param interruptible If true, the debounce handler will not be called after the valdi context is destroyed
   * @param delayMs how long to wait before calling the handler
   * @param handler the handler to call with the debounced values
   */
  constructor(
    interruptible: boolean,
    delayMs: number,
    handler: (values: T[], completion: (values?: R[], error?: any) => void) => void,
  ) {
    this.debouncer = new Debounce(interruptible, delayMs, (items: Item<T, R>[]) => {
      // Get the items values and handlers
      const values = items.map((item: Item<T, R>) => {
        return item.value;
      });
      const completions = items.map((item: Item<T, R>) => {
        return item.completion;
      });
      // Run a single handler for all values
      handler(values, (values?: R[], error?: any) => {
        // The dispatch result array to items handlers
        for (let i = 0; i < completions.length; i++) {
          const completion = completions[i];
          if (values) {
            completion(values[i], error);
          } else {
            completion(undefined, error);
          }
        }
      });
    });
  }

  schedule(value: T, completion: (values?: R, error?: any) => void) {
    this.debouncer.schedule({
      value: value,
      completion: completion,
    });
  }

  schedulePromise(value: T): Promise<R> {
    return new Promise((resolve, reject) => {
      this.schedule(value, (values, error) => {
        if (values) {
          resolve(values);
        } else {
          reject(error);
        }
      });
    });
  }

  stop() {
    this.debouncer.stop();
  }
}
