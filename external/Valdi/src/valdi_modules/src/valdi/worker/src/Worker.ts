import { ValdiRuntime, NativeWorker, OnMessageFunc } from 'valdi_core/src/ValdiRuntime';

declare const runtime: ValdiRuntime;

/** This Web Workers API interface represents a background task that can be
 * easily created and can send messages back to its creator. Creating a worker
 * is as simple as calling the Worker() constructor and specifying a script to
 * be run in the worker thread. */
export class Worker {
  private nativeWorker: NativeWorker | null;

  public constructor(url: string) {
    this.nativeWorker = runtime.createWorker(url);
  }

  public set onmessage(func: OnMessageFunc<unknown>) {
    if (this.nativeWorker) {
      this.nativeWorker.setOnMessage(func);
    }
  }

  public postMessage<T>(data: T): void {
    if (this.nativeWorker) {
      this.nativeWorker.postMessage(data);
    }
  }

  public terminate(): void {
    if (this.nativeWorker) {
      this.nativeWorker.terminate();
      this.nativeWorker = null;
    }
  }
}

export default Worker;

export function inWorker(): boolean {
  // 'location' is a global variable that is set up in a worker but not in the
  // host runtime.
  return typeof location !== 'undefined';
}
