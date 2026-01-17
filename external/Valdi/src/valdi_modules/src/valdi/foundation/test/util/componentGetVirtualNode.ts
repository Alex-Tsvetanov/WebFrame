import { IComponent } from 'valdi_core/src/IComponent';
import { IRenderedVirtualNode } from 'valdi_core/src/IRenderedVirtualNode';

export function componentGetVirtualNode(component: IComponent): IRenderedVirtualNode {
  return component.renderer.getComponentVirtualNode(component);
}
