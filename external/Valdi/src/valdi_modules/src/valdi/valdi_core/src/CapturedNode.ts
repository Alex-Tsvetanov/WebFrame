import { StringMap } from 'coreutils/src/StringMap';
import { ElementFrame } from 'valdi_tsx/src/Geometry';
import { NativeNode } from 'valdi_tsx/src/NativeNode';
import { NativeView } from 'valdi_tsx/src/NativeView';
import { ConsoleRepresentable } from './ConsoleRepresentable';
import { IComponent } from './IComponent';
import { ElementId, IRenderedElement } from './IRenderedElement';
import { IRenderedVirtualNode } from './IRenderedVirtualNode';
import { IRenderer } from './IRenderer';
import { PropertyList } from './utils/PropertyList';
import { computeUniqueId } from './utils/RenderedVirtualNodeUtils';

export interface IElementData {
  readonly id: ElementId;
  readonly tag: string;
  readonly viewClass: string;
  readonly attributes: StringMap<any>;
  readonly frame: ElementFrame;
}

/**
 * An immutable implementation of IRenderedVirtualNode.
 */
export class CapturedRenderedVirtualNode implements IRenderedVirtualNode {
  parent: IRenderedVirtualNode | undefined;
  parentIndex = 0;
  element: IRenderedElement | undefined;

  private cachedUniqueId?: string;

  constructor(
    readonly key: string,
    elementData: IElementData | undefined,
    readonly component: IComponent | undefined,
    readonly children: CapturedRenderedVirtualNode[],
    readonly renderer: IRenderer,
  ) {
    let index = 0;
    for (const child of children) {
      child.parent = this;
      child.parentIndex = index;
      index++;
    }

    if (elementData) {
      this.element = makeElement(elementData, this);
    }
  }

  get uniqueId(): string {
    if (this.cachedUniqueId) {
      return this.cachedUniqueId;
    }
    this.cachedUniqueId = computeUniqueId(this);
    return this.cachedUniqueId;
  }
}

function resolveElementChildren(virtualNodeChildren: IRenderedVirtualNode[], children: IRenderedElement[]) {
  for (const virtualNodeChild of virtualNodeChildren) {
    if (virtualNodeChild.element) {
      children.push(virtualNodeChild.element);
    } else {
      resolveElementChildren(virtualNodeChild.children, children);
    }
  }
}

function makeElement(data: IElementData, node: IRenderedVirtualNode): IRenderedElement {
  const children: IRenderedElement[] = [];
  resolveElementChildren(node.children, children);

  return new CapturedRenderedElement(data, node, children);
}

function immutableError(methodName: string): Error {
  return new Error(`Cannot use ${methodName} on CapturedRenderedElement`);
}

export class CapturedRenderedElement implements IRenderedElement, ConsoleRepresentable {
  parent: IRenderedElement | undefined;
  parentIndex = 0;

  constructor(
    readonly data: IElementData,
    readonly virtualNode: IRenderedVirtualNode,
    readonly children: IRenderedElement[],
  ) {}

  getAttribute(name: string): any {
    return this.data.attributes[name];
  }

  setAttribute(name: string, value: any): boolean {
    throw immutableError('setAttribute');
  }

  setAttributes(attributes: PropertyList): boolean {
    throw immutableError('setAttributes');
  }

  getVirtualNode(): IRenderedVirtualNode {
    return this.virtualNode;
  }

  getAttributeNames(): string[] {
    return Object.keys(this.data.attributes);
  }

  getNativeView(): Promise<NativeView | undefined> {
    return new Promise((resolve, reject) => {
      reject(immutableError('getNativeView'));
    });
  }

  getNativeNode(): NativeNode | undefined {
    return undefined;
  }

  takeSnapshot(): Promise<string | undefined> {
    return new Promise((resolve, reject) => {
      reject(immutableError('takeSnapshot'));
    });
  }

  get frame(): ElementFrame {
    return this.data.frame;
  }

  get emittingComponent(): IComponent | undefined {
    let current: IRenderedVirtualNode | undefined = this.virtualNode;
    while (current && !current.component) {
      current = current.parent;
    }

    return current?.component;
  }

  get renderer(): IRenderer {
    return this.virtualNode.renderer;
  }

  get id(): ElementId {
    return this.data.id;
  }

  get tag(): string {
    return this.data.tag;
  }

  get viewClass(): string {
    return this.data.viewClass;
  }

  get key(): string {
    return this.virtualNode.key;
  }

  toConsoleRepresentation() {
    return [this.viewClass, this.data.attributes];
  }
}
