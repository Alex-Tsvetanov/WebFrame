import { IComponent } from 'valdi_core/src/IComponent';

export function componentGetChildren(component: IComponent): IComponent[] {
  return component.renderer.getComponentChildren(component);
}
