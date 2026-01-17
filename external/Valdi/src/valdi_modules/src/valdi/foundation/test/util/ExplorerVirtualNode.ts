import { IComponent, ComponentConstructor } from 'valdi_core/src/IComponent';
import { IRenderedElement } from 'valdi_core/src/IRenderedElement';
import { IRenderedVirtualNode } from 'valdi_core/src/IRenderedVirtualNode';
import { IRenderedElementViewClass } from 'valdi_test/test/IRenderedElementViewClass';
import { virtualNodeTreeDump } from './virtualNodeTreeDump';
import 'jasmine/src/jasmine';

export class ExplorerVirtualNode {
  constructor(private virtualNode: IRenderedVirtualNode) {}

  expectToBeComponent<T extends IComponent>(clazz: ComponentConstructor<T>, viewModel?: any) {
    const component = this.virtualNode.component;
    if (!component) {
      throw new Error('Is not a component');
    }
    if (!(component instanceof clazz)) {
      throw new Error(`Is not a component of type: ${clazz.name}`);
    }
    if (viewModel) {
      for (const key of Object.keys(viewModel)) {
        const value = viewModel[key];
        const found = component.viewModel[key];
        if (found !== value) {
          throw new Error(`Wrong component attribute: ${clazz.name}.${key} -> ${found} (expected: ${value})`);
        }
      }
    }
    return this;
  }

  expectToBeElement(viewClass: IRenderedElementViewClass, attributes?: any) {
    const element = this.virtualNode.element;
    if (!element) {
      throw new Error('Is not an element');
    }
    if (element.viewClass != viewClass) {
      throw new Error(`Is not an element of type: ${viewClass} (found: ${element.viewClass})`);
    }
    if (attributes) {
      for (const key of Object.keys(attributes)) {
        const value = attributes[key];
        const found = element.getAttribute(key);
        if (found !== value) {
          throw new Error(`Wrong element attribute: ${viewClass}.${key} -> ${found} (expected: ${value})`);
        }
      }
    }
    return this;
  }

  expectNumberOfChildren(count: number) {
    const found = this.virtualNode.children.length;
    if (found != count) {
      throw new Error(`Invalid number of children: ${found} (expected: ${count})`);
    }
    return this;
  }

  expectDirectChildComponent<T extends IComponent>(
    clazz: ComponentConstructor<T>,
    viewModel?: any,
  ): ExplorerVirtualNode {
    for (const child of this.virtualNode.children) {
      if (child.component instanceof clazz) {
        const explorer = new ExplorerVirtualNode(child);
        explorer.expectToBeComponent(clazz, viewModel);
        return explorer;
      }
    }
    throw new Error(`Could not find the children component: ${clazz.name}`);
  }

  expectDirectChildElement(viewClass: IRenderedElementViewClass, attributes?: any): ExplorerVirtualNode {
    for (const child of this.virtualNode.children) {
      if (child.element?.viewClass === viewClass) {
        const explorer = new ExplorerVirtualNode(child);
        explorer.expectToBeElement(viewClass, attributes);
        return explorer;
      }
    }
    throw new Error(`Could not find the children element: ${viewClass}`);
  }

  expectExactDirectChildrenElements(params: [IRenderedElementViewClass, any][]): ExplorerVirtualNode[] {
    this.expectNumberOfChildren(params.length);
    const results = [];
    for (let i = 0; i < params.length; i++) {
      const child = this.virtualNode.children[i];
      const param = params[i];
      const explorer = new ExplorerVirtualNode(child);
      explorer.expectToBeElement(param[0], param[1]);
      results.push(explorer);
    }
    return results;
  }

  applyOnChild(index: number, cb: (explorer: ExplorerVirtualNode) => void) {
    const count = this.virtualNode.children.length;
    if (index >= count) {
      throw new Error(`Child index is larger than the number of children: ${index} (size: ${count})`);
    }
    const child = this.virtualNode.children[index];
    cb(new ExplorerVirtualNode(child));
    return this;
  }

  applyOnElement(cb: (it: IRenderedElement) => void) {
    const element = this.virtualNode.element;
    if (!element) {
      throw new Error('Is not an element');
    }
    cb(element);
    return this;
  }

  applyOnComponent<T extends IComponent>(clazz: ComponentConstructor<T>, cb: (it: T) => void) {
    const component = this.virtualNode.component;
    if (!component) {
      throw new Error('Is not a component');
    }
    if (!(component instanceof clazz)) {
      throw new Error(`Is not a component of type: ${clazz.name}`);
    }
    cb(component);
    return this;
  }

  dumpTree() {
    virtualNodeTreeDump(this.virtualNode);
    return this;
  }
}
