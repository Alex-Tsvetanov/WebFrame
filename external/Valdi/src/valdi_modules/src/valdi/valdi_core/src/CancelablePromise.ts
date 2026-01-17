/**
 * Represents a promise object that can be canceled.
 */
export interface CancelablePromise<T> extends PromiseLike<T> {
  cancel?(): void;
}

/**
 * Return a CancelablePromise from an existing promise, with a cancel callback that will
 * be invoked when the cancelable promise is canceled.
 */
export function promiseToCancelablePromise<T>(promise: Promise<T>, onCancel: () => void): CancelablePromise<T> {
  return {
    cancel: onCancel,
    then: (onfulfilled, onrejected) => {
      return promise.then(onfulfilled, onrejected);
    },
  };
}

export type PromiseOnCancelFn = () => void;

export class PromiseCanceler {
  /**
   * Returns whether cancel() has been called on the promise canceler.
   */
  get canceled(): boolean {
    return this._canceled === true;
  }

  private _canceled?: boolean;
  private _onCancels?: PromiseOnCancelFn | PromiseOnCancelFn[];

  constructor() {}

  /**
   * Registers a cancel function that will be invoked when cancel() is called.
   */
  onCancel(fn: PromiseOnCancelFn): void {
    if (this._canceled) {
      fn();
      return;
    }

    if (!this._onCancels) {
      this._onCancels = fn;
    } else {
      if (Array.isArray(this._onCancels)) {
        this._onCancels.push(fn);
      } else {
        this._onCancels = [this._onCancels, fn];
      }
    }
  }

  /**
   * Mark the CancelablePromise as canceled, which will notify all the
   * registered onCancel callbacks.
   */
  cancel(): void {
    if (!this._canceled) {
      this._canceled = true;

      const onCancels = this._onCancels;
      if (!onCancels) {
        return;
      }

      this._onCancels = undefined;

      if (Array.isArray(onCancels)) {
        for (const onCancel of onCancels) {
          onCancel();
        }
      } else {
        onCancels();
      }
    }
  }

  clear() {
    this._onCancels = undefined;
  }

  toCancelablePromise<T>(promise: Promise<T>): CancelablePromise<T> {
    const cancelablePromise = promise as CancelablePromise<T>;
    cancelablePromise.cancel = this.cancel.bind(this);
    return cancelablePromise;
  }
}
