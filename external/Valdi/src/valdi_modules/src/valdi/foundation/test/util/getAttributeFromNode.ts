import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { isRenderedElement } from './isRenderedElement';

export const getAttributeFromNode = <T = any>(node: IComponent | IRenderedElement, key: string): T | undefined => {
  if (isRenderedElement(node)) {
    return node.getAttribute(key);
  }
  return node.viewModel[key];
};
