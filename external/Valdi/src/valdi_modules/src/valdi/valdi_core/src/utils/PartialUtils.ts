/**
 * Merges a partial into the given object.
 * Returns undefined if the partial did not
 * cause into to be mutated, otherwise returns
 * a new object with the partial merged.
 */
export function mergePartial<T>(partial: Partial<T>, into: T | undefined): T | undefined {
  if (!into) {
    return <T>{ ...partial };
  }
  let objectCopy: T | undefined;
  for (const key in partial) {
    const newValue = partial[key];
    const oldValue = into[key];

    if (oldValue !== newValue) {
      if (!objectCopy) {
        objectCopy = { ...into };
      }
      objectCopy[key] = newValue!;
    }
  }

  return objectCopy;
}
