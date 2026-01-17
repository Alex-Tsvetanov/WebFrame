export function mockObject<T>(input: Partial<T> = {}): T {
  return input as T;
}
