import { CancelablePromise } from 'valdi_core/src/CancelablePromise';
import { ICancelable } from '../ICancelable';
import CallbackCancelable from './CallbackCancelable';

/**
 * Allows ICancelables to be grouped, so they can be canceled together.
 */
export default class CancelableGroup implements ICancelable {
  private canceled = false;
  private cancelables: ICancelable[];

  constructor(...cancelables: readonly ICancelable[]) {
    this.cancelables = [...cancelables];
  }

  get isCanceled(): boolean {
    return this.canceled;
  }

  add = (cancelable: ICancelable) => {
    if (this.canceled) {
      cancelable.cancel();
      return this;
    }
    this.cancelables.push(cancelable);
    return this;
  };

  addCancelablePromise = <T>(cancelablePromise: CancelablePromise<T>): Promise<T> => {
    this.add(new CallbackCancelable(cancelablePromise.cancel));
    return Promise.resolve(cancelablePromise);
  };

  cancel = () => {
    this.canceled = true;
    const cancelables = this.cancelables;
    this.cancelables = [];
    for (const cancelable of cancelables) {
      cancelable.cancel();
    }
  };
}
