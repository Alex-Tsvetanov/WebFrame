import { ICancelable } from '../ICancelable';

/**
 * An implementation of cancelable that calls a given callback once when canceled for the first time.
 */
export default class CallbackCancelable implements ICancelable {
  private callback: (() => void) | undefined;

  constructor(callback?: () => void) {
    this.callback = callback;
  }

  cancel = () => {
    const callback = this.callback;
    this.callback = undefined;
    callback?.();
  };
}
