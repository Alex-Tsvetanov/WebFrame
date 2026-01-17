import { PropertyList } from './utils/PropertyList';

export type ViewClass = string;

let idSequence = 0;

export class NodePrototype {
  readonly id: string;
  readonly tag: string;
  readonly viewClass: ViewClass;
  readonly attributes: PropertyList | undefined;

  constructor(tag: string, viewClass: ViewClass, attributes?: PropertyList) {
    this.id = '__node' + ++idSequence;
    this.tag = tag;
    this.viewClass = viewClass;
    this.attributes = attributes;
  }
}

export class DeferredNodePrototype extends NodePrototype {
  constructor(tag: string, attributes?: PropertyList) {
    super(tag, 'Deferred', attributes);
  }
}
