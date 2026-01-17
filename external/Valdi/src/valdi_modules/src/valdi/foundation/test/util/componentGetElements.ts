import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';

export function componentGetElements(component: IComponent | IComponent[]): IRenderedElement[] {
  if (component instanceof Array) {
    let results: IRenderedElement[] = [];
    for (const item of component) {
      results = results.concat(item.renderer.getComponentRootElements(item, false));
    }
    return results;
  } else {
    return component.renderer.getComponentRootElements(component, false);
  }
}
