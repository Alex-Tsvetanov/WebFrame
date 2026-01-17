import { ValdiRuntime } from "valdi_core/src/ValdiRuntime";

declare const runtime: ValdiRuntime;

export function emitUncaughtError(handler: (error: unknown, contextId: string) => number) {
  runtime.setUncaughtExceptionHandler(handler);

  setTimeout(() => {
    throw new Error('This has thrown an error');
  });
}

export function emitRejectedPromise(handler: (error: unknown, contextId: string) => number) {
  runtime.setUnhandledRejectionHandler(handler);

  new Promise<void>((resolve, reject) => {
      reject(new Error('This promise has failed'));
  });
}
