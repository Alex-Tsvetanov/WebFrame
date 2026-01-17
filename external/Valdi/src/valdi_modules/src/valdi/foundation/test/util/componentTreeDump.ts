import { IComponent } from 'valdi_core/src/IComponent';
import { componentGetChildren } from './componentGetChildren';
import { componentGetKey } from './componentGetKey';
import { consoleDim, consoleFgRed, consoleFgYellow, consoleReset } from './consoleColor';

interface Options {
  maxDepth?: number;
  noColor?: boolean;
  maxSpace?: number;
  maxSpacePerValue?: number;
  blockKeys?: string[];
}

/**
 * Print recursively the component tree of the component
 */
export function componentTreeDump(component: IComponent | IComponent[], options: Options = {}) {
  const { maxDepth = 100, noColor = false, maxSpace = 50, maxSpacePerValue = 50, blockKeys = [] } = options;
  const colorValue = (value: string, color: string) => {
    if (noColor) {
      return value;
    }
    return color + value + consoleReset;
  };

  const limit = maxDepth;
  const recursor = (current: IComponent, depth: number, looping: Map<number, boolean>) => {
    if (depth > limit) {
      return;
    }
    const key = componentGetKey(current);
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
    const keys = Object.keys(current.viewModel);
    keys.sort((a, b) => {
      return a.length - b.length;
    });
    let usedSpace = 0;
    for (const key of keys) {
      if (blockKeys.includes(key)) {
        continue;
      }
      const value = current.viewModel[key];
      const valueString = stringifyValue(value);
      if (valueString !== undefined) {
        const trimmedValueString =
          valueString.length > maxSpacePerValue ? valueString.slice(0, maxSpacePerValue) + '..."' : valueString;
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

    const children = componentGetChildren(current);
    for (let i = 0; i < children.length; i++) {
      const newLooping = new Map(looping);
      if (i + 1 < children.length) {
        newLooping.set(depth, true);
      }
      recursor(children[i], depth + 1, newLooping);
    }
  };

  if (component instanceof Array) {
    for (const item of component) {
      recursor(item, 0, new Map());
    }
  } else {
    recursor(component, 0, new Map());
  }
}

const stringifyValue = (value: unknown): string | undefined => {
  // Primitive types
  if (value === null || typeof value !== 'object') {
    return JSON.stringify(value);
  }
  if (value instanceof Date) {
    return value.toISOString();
  }
  if (value instanceof Array) {
    return `[${value.map(stringifyValue).join(', ')}]`;
  }
  // Plain object
  if (value.constructor === Object) {
    const cache: unknown[] = [];
    return JSON.stringify(value, (key, value) =>
      typeof value === 'object' && value !== null
        ? cache.includes(value)
          ? undefined // Duplicate reference found, discard key
          : cache.push(value) && value // Store value in our collection
        : value,
    );
  }
  return undefined;
};
