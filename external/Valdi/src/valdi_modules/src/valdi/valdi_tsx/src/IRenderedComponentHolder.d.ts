import { IComponentBase } from './IComponentBase';

export interface IRenderedComponentHolder<ComponentT extends IComponentBase, VirtualNodeT> {
  onComponentDidCreate(component: ComponentT, virtualNode: VirtualNodeT): void;
  onComponentWillDestroy(component: ComponentT, virtualNode: VirtualNodeT): void;
}
