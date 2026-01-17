import { ComponentConstructor, IComponent } from 'valdi_core/src/IComponent';
import { getChildComponentsOfType } from 'valdi_core/src/utils/ComponentUtils';

export function componentTypeFind<T extends IComponent>(
  component: IComponent | IComponent[],
  clazz: ComponentConstructor<T>,
): T[] {
  let results: T[] = [];
  if (component instanceof Array) {
    for (const item of component) {
      results = results.concat(getChildComponentsOfType(item, clazz, true));
    }
  } else {
    results = results.concat(getChildComponentsOfType(component, clazz, true));
  }
  return results;
}
