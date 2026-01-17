type Truthy<T> = T extends false | '' | 0 | null | undefined ? never : T; // from lodash

export function noop() {}

/** Used to perform type-correct truthy `.filter` ops. */
export function truthy<T>(value: T): value is Truthy<T> {
  return !!value;
}

/** Returns passed in `obj` */
export function identity<T>(obj: T): T {
  return obj;
}

/** Used to perform type-correct nonnull `.filter` ops. */
export function nonnull<T>(value: T): value is NonNullable<T> {
  return value != null;
}
