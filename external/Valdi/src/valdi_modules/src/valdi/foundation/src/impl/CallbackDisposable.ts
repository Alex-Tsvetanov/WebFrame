import { ICancelable } from '../ICancelable';
import { IDisposable } from '../IDisposable';
import CallbackCancelable from './CallbackCancelable';

/**
 * An implementation of disposable that calls a given callback once when disposed for the first time.
 */
export default class CallbackDisposable implements IDisposable {
  private readonly cancelable: ICancelable;

  constructor(callback: () => void) {
    // Disposables and cancelables are identical in all but name, so just use
    // a cancelable under the hood
    this.cancelable = new CallbackCancelable(callback);
  }

  dispose(): void {
    this.cancelable.cancel();
  }
}
