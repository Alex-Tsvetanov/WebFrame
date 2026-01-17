export function receiveError(cb: () => void): string | undefined {
  try {
    cb();
    return undefined;
  } catch (err: any) {
    return err.message;
  }
}

export function throwError() {
  throw new Error('This is an error');
}
