import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';

export const isRenderedElement = (node: IComponent | IRenderedElement): node is IRenderedElement => {
  return (node as IRenderedElement).viewClass !== undefined;
};
