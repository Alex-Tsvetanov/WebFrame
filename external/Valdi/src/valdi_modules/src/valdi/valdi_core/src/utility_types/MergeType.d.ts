/**
 * Merge a type union (of objects) into a single object type.
 *
 * See: https://dev.to/lucianbc/union-type-merging-in-typescript-9al
 *
 * Useful when working with a union type object as a "collapsed" bag of all possible properties (that are optional).
 * We use this for an AllAnimationOptions type.
 * */
export type MergeType<T extends object> = {
  [k in CommonKeys<T>]: PickTypeOf<T, k>;
} & {
  [k in NonCommonKeys<T>]?: PickTypeOf<T, k>;
};

/**
 * Machinery necessary to implement the MergeType type.
 *
 * See: https://dev.to/lucianbc/union-type-merging-in-typescript-9al
 *
 */
export type CommonKeys<T extends object> = keyof T;

export type AllKeys<T> = T extends any ? keyof T : never;

export type Subtract<A, C> = A extends C ? never : A;
export type NonCommonKeys<T extends object> = Subtract<AllKeys<T>, CommonKeys<T>>;

export type PickType<T, K extends AllKeys<T>> = T extends { [k in K]?: any } ? T[K] : undefined;

export type PickTypeOf<T, K extends string | number | symbol> = K extends AllKeys<T> ? PickType<T, K> : never;
