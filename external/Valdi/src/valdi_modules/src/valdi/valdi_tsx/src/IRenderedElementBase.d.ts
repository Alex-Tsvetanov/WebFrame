export type StringKeyOf<T> = Extract<keyof T, string>;

export type AttributesFrom<T> = { [K in StringKeyOf<T>]?: T[K] };

export interface IRenderedElementBase<T = unknown> {
  /**
   * Return a copy of the attribute names held by this element.
   */
  getAttributeNames(): (keyof AttributesFrom<T>)[];
}
