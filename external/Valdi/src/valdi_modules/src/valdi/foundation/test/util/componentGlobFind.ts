import { IComponent } from 'valdi_core/src/IComponent';
import { componentGetChildren } from './componentGetChildren';
import { componentGetKey } from './componentGetKey';
import { globToRegex } from './globToRegex';

/**
 * Find components by globbing their relative path
 * Allow search by glob wildcard (like in file system)
 */
export function componentGlobFind(component: IComponent | IComponent[], pattern: string): IComponent[] {
  const possibilities: { path: string; component: IComponent }[] = [];

  const recursor = (current: IComponent, path: string) => {
    const currentName = componentGetKey(current);
    const newPath = path + currentName;
    possibilities.push({ path: newPath, component: current });
    for (const child of componentGetChildren(current)) {
      recursor(child, newPath + '/');
    }
  };

  if (component instanceof Array) {
    for (const item of component) {
      recursor(item, '');
    }
  } else {
    recursor(component, '');
  }

  const regex = globToRegex(pattern);
  const results: IComponent[] = [];
  for (const possibility of possibilities) {
    if (regex.test(possibility.path)) {
      results.push(possibility.component);
    }
  }
  return results;
}
