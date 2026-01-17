import { GetOrCreateSourceMapFunc, symbolicateStack } from 'source_map/src/StackSymbolicator';
import { getModuleLoader } from './ModuleLoaderGlobal';
import { SymbolicatedError } from './SymbolicatedError';

let symbolicateStackFunction:
  | ((stack: string, getOrCreateSourceMapFunc: GetOrCreateSourceMapFunc) => string)
  | undefined;
try {
  symbolicateStackFunction = symbolicateStack;
} catch (err: any) {
  console.log(`symbolicateStackFunction error: ${err.message}`);
}

/**
 * Parse and symbolicate an error stack such that lines reported
 * within the stack are mapped to the original TypeScript files.
 */
export function symbolicateErrorStack(stack: string): string {
  if (!symbolicateStackFunction) {
    return stack;
  }

  return symbolicateStackFunction(stack, getModuleLoader().getOrCreateSourceMap);
}

export function symbolicate(error: Error | SymbolicatedError): SymbolicatedError {
  if (!symbolicateStackFunction || !error.stack) {
    return error;
  }

  if ((error as SymbolicatedError).originalStack) {
    return error;
  }

  const symbolicatedStack = symbolicateErrorStack(error.stack);

  const newError = new (error as any).constructor(error.message) as SymbolicatedError;
  newError.stack = symbolicatedStack;
  newError.originalStack = error.stack;

  return newError;
}
