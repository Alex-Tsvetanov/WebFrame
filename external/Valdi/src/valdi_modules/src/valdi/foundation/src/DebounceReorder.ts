import { Debounce } from './Debounce';

interface Item<T> {
  priority: number;
  value: T;
}

/**
 * Allow the batching of multiple async operations
 * by immediately scheduling them
 * and automatically running them all in a new order
 * after a specific debounced timer
 */
export class DebounceReorder<T> {
  private debouncer: Debounce<Item<T>>;

  /**
   * Create a DebounceReorder object
   * @param interruptible If true, the debounce handler will not be called after the valdi context is destroyed
   * @param delayMs how long to wait before calling the handler
   * @param handler the handler to call with the debounced values
   */
  constructor(interruptible: boolean, delayMs: number, handler: (value: T) => void) {
    this.debouncer = new Debounce(interruptible, delayMs, (items: Item<T>[]) => {
      items.sort((a, b) => {
        return a.priority - b.priority;
      });
      return items.forEach(i => handler(i.value));
    });
  }

  schedule(priority: number, value: T) {
    this.debouncer.schedule({
      priority: priority,
      value: value,
    });
  }

  stop() {
    this.debouncer.stop();
  }
}
