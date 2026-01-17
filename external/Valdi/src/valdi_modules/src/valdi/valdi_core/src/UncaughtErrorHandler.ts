import { remove } from 'coreutils/src/ArrayUtils';
import { ValdiRuntime, ExceptionHandlerResult } from './ValdiRuntime';
import { IComponent } from './IComponent';

declare const runtime: ValdiRuntime;

export const enum UncaughtErrorHandlerResult {
  /**
   * Keep the default behavior for unhandled errors within the runtime.
   */
  USE_DEFAULT,
  /**
   * Ignore the error. This should be used only if the error handler
   * fully processed the error, and there is no expectation of the
   * error being further processed by the runtime.
   */
  IGNORE,
  /**
   * Crash the application as a result of that error.
   */
  CRASH_APPLICATION,
}

/**
 * Callback that will be called whenever an uncaught error or an uncaught rejected
 * promise is detected within the runtime.
 */
export type UncaughtErrorHandler = (isHandledRejectedPromise: boolean, error: unknown) => UncaughtErrorHandlerResult;

interface ErrorHandlersProvider {
  errorHandlersByContextId?: { [contextId: string]: UncaughtErrorHandler[] };
  catchAllErrorHandler?: UncaughtErrorHandler;
}

declare const global: ErrorHandlersProvider;

function doCallErrorHandler(
  errorHandler: UncaughtErrorHandler,
  isHandledRejectedPromise: boolean,
  error: unknown,
): number {
  const result = errorHandler(isHandledRejectedPromise, error);
  switch (result) {
    case UncaughtErrorHandlerResult.USE_DEFAULT:
      return ExceptionHandlerResult.NOTIFY;
    case UncaughtErrorHandlerResult.IGNORE:
      return ExceptionHandlerResult.IGNORE;
    case UncaughtErrorHandlerResult.CRASH_APPLICATION:
      return ExceptionHandlerResult.CRASH;
  }
}

function callErrorHandlers(
  contextId: string,
  isHandledRejectedPromise: boolean,
  error: unknown,
): ExceptionHandlerResult {
  const catchAllErrorHandler = global.catchAllErrorHandler;
  if (catchAllErrorHandler) {
    return doCallErrorHandler(catchAllErrorHandler, isHandledRejectedPromise, error);
  }

  if (contextId === undefined) {
    // Runtime Backward compatibility
    return ExceptionHandlerResult.NOTIFY;
  }

  const handlers = global.errorHandlersByContextId?.[contextId];
  if (!handlers?.length) {
    return ExceptionHandlerResult.NOTIFY;
  }

  const handler = handlers[handlers.length - 1];
  return doCallErrorHandler(handler, isHandledRejectedPromise, error);
}

runtime.setUncaughtExceptionHandler((error, contextId) => {
  return callErrorHandlers(contextId, false, error);
});

runtime.setUnhandledRejectionHandler((promiseResult, contextId) => {
  return callErrorHandlers(contextId, true, promiseResult);
});

/**
 * Set or unset a catch all error handler which will be resolved by default
 * for any uncaught errors.
 */
export function setCatchAllUncaughtErrorHandler(errorHandler: UncaughtErrorHandler | undefined) {
  global.catchAllErrorHandler = errorHandler;
}

/**
 * Register an error handler for the given contextId. The handler will be called
 * whenever an uncaught exception or an unhandled rejected promise is found.
 * @returns a dispose function, which should be called to unregister the error handler
 */
export function registerUncaughtErrorHandlerForContextId(
  contextId: string,
  errorHandler: UncaughtErrorHandler,
): () => void {
  if (!global.errorHandlersByContextId) {
    global.errorHandlersByContextId = {};
  }
  let handlers = global.errorHandlersByContextId[contextId];
  if (!handlers) {
    handlers = [];
    global.errorHandlersByContextId[contextId] = handlers;
  }
  handlers.push(errorHandler);

  return () => {
    if (!global.errorHandlersByContextId) {
      return;
    }
    const handlers = global.errorHandlersByContextId[contextId];
    if (handlers) {
      if (remove(handlers, errorHandler) && handlers.length === 0) {
        delete global.errorHandlersByContextId[contextId];
      }
    }
  };
}

/**
 * Register an error handler for the given component. The handler will be called
 * whenever an uncaught exception or an unhandled rejected promise is found.
 * The error handler applies for the entire renderer tree of the component.
 * @returns a dispose function, which should be called to unregister the error handler
 */
export function registerUncaughtErrorHandlerForComponent(
  component: IComponent,
  errorHandler: UncaughtErrorHandler,
): () => void {
  return registerUncaughtErrorHandlerForContextId(component.renderer.contextId, errorHandler);
}
