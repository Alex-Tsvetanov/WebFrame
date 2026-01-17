import { setTimeoutConfigurable } from 'valdi_core/src/SetTimeout';

export class Debounce<T> {
  private interruptible: boolean;
  private delayMs: number;
  private handler: (values: T[]) => void;
  private closed: boolean;

  private timeout: number | undefined;
  private queue: T[] = [];

  /**
   * Create a Debounce object
   * @param interruptible If true, the debounce handler will not be called after the valdi context is destroyed
   * @param delayMs how long to wait before calling the handler
   * @param handler the handler to call with the debounced values
   */
  constructor(interruptible: boolean, delayMs: number, handler: (values: T[]) => void) {
    this.interruptible = interruptible;
    this.delayMs = delayMs;
    this.handler = handler;
    this.closed = false;
  }

  schedule(value: T) {
    if (this.closed) {
      return;
    }
    this.queue.push(value);
    if (this.timeout) {
      clearTimeout(this.timeout);
    }
    this.timeout = setTimeoutConfigurable(
      this.interruptible,
      () => {
        this.flush();
      },
      this.delayMs,
    );
  }

  private flush() {
    this.handler(this.queue);
    this.queue = [];
  }

  stop() {
    this.closed = true;
    this.flush();
    if (this.timeout) {
      clearTimeout(this.timeout);
    }
    this.timeout = undefined;
  }
}
