import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { consoleDim, consoleFgRed, consoleFgYellow, consoleReset } from './consoleColor';

/**
 * Print recursively the element tree of the element
 */
export function elementTreeDump(element: IRenderedElement | IRenderedElement[], maxDepth?: number, noColor?: boolean) {
  const colorValue = (value: string, color: string) => {
    if (noColor) {
      return value;
    }
    return color + value + consoleReset;
  };

  const limit = maxDepth ?? 100;
  const recursor = (current: IRenderedElement, depth: number, looping: Map<number, boolean>) => {
    if (depth > limit) {
      return;
    }
    const key = current.viewClass + ':' + current.key;
    let prefix = '';
    for (let i = 0; i < depth; i++) {
      if (i + 1 == depth) {
        prefix += ' +';
      } else {
        if (looping.get(i)) {
          prefix += ' |';
        } else {
          prefix += '  ';
        }
      }
    }

    const simplifiedViewModel: string[] = [];
    const keys = current.getAttributeNames();
    keys.sort((a, b) => {
      return a.length - b.length;
    });
    const maxSpace = 50;
    let usedSpace = 0;
    for (const key of keys) {
      const value = current.getAttribute(key);
      const type = typeof value;
      if (type == 'number' || type == 'string' || type == 'boolean') {
        const valueString = value.toString();
        const itemString = key + ': ' + valueString;
        if (itemString.length + usedSpace > maxSpace) {
          simplifiedViewModel.push('...');
          break;
        } else {
          usedSpace += itemString.length;
          simplifiedViewModel.push(itemString);
        }
      }
    }

    console.log(
      colorValue(prefix, consoleFgYellow),
      colorValue(key, consoleFgRed),
      colorValue('{' + simplifiedViewModel.join(', ') + '}', consoleDim),
    );

    const children = current.children;
    for (let i = 0; i < children.length; i++) {
      const newLooping = new Map(looping);
      if (i + 1 < children.length) {
        newLooping.set(depth, true);
      }
      recursor(children[i], depth + 1, newLooping);
    }
  };

  if (element instanceof Array) {
    for (const item of element) {
      recursor(item, 0, new Map());
    }
  } else {
    recursor(element, 0, new Map());
  }
}
