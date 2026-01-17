import { IRenderedElementHolder } from 'valdi_tsx/src/IRenderedElementHolder';
import { AttributesFrom } from './IRenderedElement';

export interface IRenderedElementAttributeApplier<T> extends IRenderedElementHolder<T> {
  setAttribute(
    attributeName: keyof AttributesFrom<T>,
    attributeValue: AttributesFrom<T>[keyof AttributesFrom<T>],
  ): void;
}
