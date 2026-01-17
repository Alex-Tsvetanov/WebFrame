import { IRenderedVirtualNode } from 'valdi_core/src/IRenderedVirtualNode';
import { componentGetKey } from './componentGetKey';

export function virtualNodeGetKey(virtualNode: IRenderedVirtualNode): string {
  const component = virtualNode.component;
  const element = virtualNode.element;
  return component ? componentGetKey(component) : element?.viewClass ?? 'element';
}
