import { IRenderedVirtualNode } from 'valdi_core/src/IRenderedVirtualNode';
import { consoleDim, consoleFgBlue, consoleFgRed, consoleFgYellow, consoleReset } from './consoleColor';
import { virtualNodeGetKey } from './virtualNodeGetKey';

/**
 * Print recursively the tree of the virtual node
 */
export function virtualNodeTreeDump(
  virtualNode: IRenderedVirtualNode | IRenderedVirtualNode[],
  maxDepth?: number,
  noColor?: boolean,
) {
  const colorValue = (value: string, color: string) => {
    if (noColor) {
      return value;
    }
    return color + value + consoleReset;
  };

  const limit = maxDepth ?? 100;
  const recursor = (current: IRenderedVirtualNode, depth: number, looping: Map<number, boolean>) => {
    if (depth > limit) {
      return;
    }

    const component = current.component;
    const element = current.element;

    const key = virtualNodeGetKey(current);
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
    const keys = component ? Object.keys(component.viewModel) : element?.getAttributeNames() ?? [];
    keys.sort((a, b) => {
      return a.length - b.length;
    });
    const maxSpace = 1000;
    let usedSpace = 0;
    for (const key of keys) {
      const value = component ? component.viewModel[key] : element?.getAttribute(key);
      const type = typeof value;
      if (type === 'number' || type === 'string' || type === 'boolean' || type === 'function') {
        let valueString = value.toString();
        if (type === 'function') {
          valueString = '<function/>';
        }
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
      colorValue(key, component ? consoleFgRed : consoleFgBlue),
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

  if (virtualNode instanceof Array) {
    for (const item of virtualNode) {
      recursor(item, 0, new Map());
    }
  } else {
    recursor(virtualNode, 0, new Map());
  }
}
