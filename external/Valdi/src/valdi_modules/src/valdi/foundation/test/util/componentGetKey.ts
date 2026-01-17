import { IComponent } from 'valdi_core/src/IComponent';

export function componentGetKey(component: IComponent): string {
  return component.renderer.getComponentKey(component);
}
