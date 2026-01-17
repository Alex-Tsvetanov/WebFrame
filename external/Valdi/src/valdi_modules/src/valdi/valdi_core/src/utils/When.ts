type NotFalse<Type> = Type extends false ? never : Type;

/** Evaluates and returns the output of `fn` when `val` is truthy, otherwise returns `undefined`. */
export function when<In, Out>(val: In, fn: (val: NotFalse<NonNullable<In>>) => Out | undefined) {
  if (!val) {
    return undefined;
  }
  return fn(val as NotFalse<NonNullable<In>>);
}

/** Evaluates and returns the output of `fn` when `val` is not undefined and not null, otherwise returns `undefined`. */
export function whenDefined<In, Out>(val: In, fn: (val: NonNullable<In>) => Out | undefined) {
  if (val === undefined || val === null) {
    return undefined;
  }

  return fn(val!);
}
