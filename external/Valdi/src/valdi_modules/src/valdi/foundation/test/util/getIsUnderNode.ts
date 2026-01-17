import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { componentGetChildren } from './componentGetChildren';
import { componentGetElements } from './componentGetElements';
import { isRenderedElement } from './isRenderedElement';

type Node = IComponent | IRenderedElement;

/**
 * Find if the instance of given component exist under another component
 */
export const getIsUnderNode = (root: Node, target: Node): boolean => {
  const isElement = isRenderedElement(root);
  if (root === target) {
    return true;
  }
  // Check elements under the root
  if (!isElement && isRenderedElement(target)) {
    const elements = componentGetElements(root);
    for (const child of elements) {
      if (getIsUnderNode(child, target)) {
        return true;
      }
    }
  }
  // Check children under the root
  const children = isElement ? root.children : componentGetChildren(root);
  for (const child of children) {
    if (getIsUnderNode(child, target)) {
      return true;
    }
  }
  return false;
};
