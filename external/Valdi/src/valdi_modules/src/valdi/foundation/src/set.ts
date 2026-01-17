export function symmetricDifference<T>(s1: Set<T>, s2: Set<T>): Set<T> {
  const diff = new Set(s1);
  for (const item of Array.from(s2)) {
    if (diff.has(item)) {
      diff.delete(item);
    } else {
      diff.add(item);
    }
  }
  return diff;
}

export function setsEquivalent<T>(s1: Set<T>, s2: Set<T>): boolean {
  if (s1 === s2) return true;
  if (s1.size !== s2.size) return false;

  for (const item of Array.from(s1)) {
    if (!s2.has(item)) {
      return false;
    }
  }
  return true;
}

/**
 * Computes predicate over a set returning the results in a new Map
 */
export function setToMap<T, U>(set: Set<T>, predicate: (item: T) => U): Map<T, U> {
  const m = new Map<T, U>();
  set.forEach(k => m.set(k, predicate(k)));
  return m;
}
