/**
 * Returns an Error object from an opaque error object.
 * @param error
 */
export function toError(error: any): Error {
  if (error instanceof Error) {
    return error;
  }
  if (error.message) {
    return new Error(error.message);
  }
  if (typeof error === 'string') {
    return new Error(error);
  }
  if (typeof error === 'object') {
    return new Error(JSON.stringify(error));
  }
  return new Error(error);
}
