export function byString<T>(selector: (v: T) => string): (l: T, r: T) => number {
  return (l, r) => selector(l).localeCompare(selector(r));
}