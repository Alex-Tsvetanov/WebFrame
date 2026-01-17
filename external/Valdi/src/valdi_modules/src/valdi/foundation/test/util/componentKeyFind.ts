import { IComponent } from 'valdi_core/src/IComponent';
import { componentGetChildren } from './componentGetChildren';
import { componentGetKey } from './componentGetKey';

/**
 * Find components with an arbitrary key recursively
 */
export function componentKeyFind(component: IComponent | IComponent[], key: string): IComponent[] {
  const results: IComponent[] = [];

  const recursor = (current: IComponent) => {
    if (componentGetKey(current) === key) {
      results.push(current);
    }
    for (const child of componentGetChildren(current)) {
      recursor(child);
    }
  };

  if (component instanceof Array) {
    for (const item of component) {
      recursor(item);
    }
  } else {
    recursor(component);
  }

  return results;
}
