export function rethrow(messagePrefix: string, error: unknown): never {
  if (error instanceof Error) {
    const newError = new Error(messagePrefix + ': ' + error.message);
    newError.stack = error.stack;
    throw newError;
  } else {
    throw new Error(messagePrefix);
  }
}
