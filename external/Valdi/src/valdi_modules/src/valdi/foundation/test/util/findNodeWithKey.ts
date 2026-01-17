import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { componentGetChildren } from './componentGetChildren';
import { componentGetElements } from './componentGetElements';
import { componentGetKey } from './componentGetKey';
import { isRenderedElement } from './isRenderedElement';

type Node = IComponent | IRenderedElement;

/**
 * Find all components or elements with a specific key recursively.
 * This supports searching in the tree of mixed components and elements.
 */
export const findNodeWithKey = (root: Node | Node[] | (() => Node | Node[]), key: string): Node[] => {
  const results: Node[] = [];

  const recursor = (current: Node) => {
    const isElement = isRenderedElement(current);
    const currentkey = isElement ? current.key : componentGetKey(current);
    if (currentkey === key) {
      results.push(current);
    }
    if (!isElement) {
      const elements = componentGetElements(current);
      elements.forEach(recursor);
    }
    const children = isElement ? current.children : componentGetChildren(current);
    for (const child of children) {
      recursor(child);
    }
  };

  const resolvedRoot = typeof root === 'function' ? root() : root;
  if (resolvedRoot instanceof Array) {
    for (const item of resolvedRoot) {
      recursor(item);
    }
  } else {
    recursor(resolvedRoot);
  }

  return results;
};
