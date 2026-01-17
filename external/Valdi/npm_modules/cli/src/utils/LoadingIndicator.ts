// We use dynamic imports here because ora uses
// the ES6 import syntax, which cannot be fixed
// with require() style imports on older node js versions.

// eslint-disable-next-line @typescript-eslint/consistent-type-imports, @typescript-eslint/no-implied-eval
const ora = new Function("return import('ora')") as () => Promise<typeof import('ora')>;

export class LoadingIndicator<T> {
  private text = '';
  private successText: string | undefined;
  private failureText: string | undefined;

  constructor(private readonly runnable: () => PromiseLike<T>) {}

  static fromTask<T>(task: PromiseLike<T>): LoadingIndicator<T> {
    return new LoadingIndicator<T>(() => task);
  }

  setText(text: string): LoadingIndicator<T> {
    this.text = text;
    return this;
  }

  setSuccessText(successText: string): LoadingIndicator<T> {
    this.successText = successText;
    return this;
  }

  setFailureText(failureText: string): LoadingIndicator<T> {
    this.failureText = failureText;
    return this;
  }

  show(): Promise<T> {
    return ora().then(ora => {
      return ora.oraPromise(this.runnable(), {
        text: this.text,
        successText: this.successText,
        failText: this.failureText,
      });
    });
  }
}
