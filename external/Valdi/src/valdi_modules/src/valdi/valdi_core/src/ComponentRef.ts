import { StringMap } from 'coreutils/src/StringMap';
import { IRenderedComponentHolder } from 'valdi_tsx/src/IRenderedComponentHolder';
import { IComponent } from './IComponent';
import { IRenderedVirtualNode } from './IRenderedVirtualNode';

function compareComponents<ComponentT extends IComponent>(left: ComponentT, right: ComponentT): number {
  const leftParentIndex = left.renderer.getComponentVirtualNode(left).parentIndex;
  const rightParentIndex = right.renderer.getComponentVirtualNode(right).parentIndex;
  return leftParentIndex - rightParentIndex;
}

export class ComponentRef<ComponentT extends IComponent>
  implements IRenderedComponentHolder<ComponentT, IRenderedVirtualNode>
{
  private componentByKey: StringMap<ComponentT> = {};
  private componentByUniqueId: StringMap<ComponentT> = {};

  private _cachedAllComponents?: ComponentT[];

  all(): ComponentT[] {
    let items = this._cachedAllComponents;
    if (!items) {
      const components: ComponentT[] = [];

      for (const key in this.componentByUniqueId) {
        const component = this.componentByUniqueId[key];
        if (component) {
          components.push(component);
        }
      }

      if (components.length > 1) {
        components.sort(compareComponents);
      }
      items = components;
      this._cachedAllComponents = components;
    }
    return items;
  }

  single(): ComponentT | undefined {
    const items = this.all();
    if (!items.length) {
      return undefined;
    }
    if (items.length > 1) {
      throw Error(`Cannot get a single component from a ref which has ${items.length} elements`);
    }

    return items[0];
  }

  getForKey(key: string): ComponentT | undefined {
    return this.componentByKey[key];
  }

  onComponentDidCreate(component: ComponentT, virtualNode: IRenderedVirtualNode): void {
    this._cachedAllComponents = undefined;
    this.componentByKey[virtualNode.key] = component;
    this.componentByUniqueId[virtualNode.uniqueId] = component;
  }
  onComponentWillDestroy(component: ComponentT, virtualNode: IRenderedVirtualNode): void {
    this._cachedAllComponents = undefined;
    delete this.componentByKey[virtualNode.key];
    delete this.componentByUniqueId[virtualNode.uniqueId];
  }
}
