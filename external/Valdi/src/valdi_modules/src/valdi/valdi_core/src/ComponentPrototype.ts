import { PropertyList } from './utils/PropertyList';

let idSequence = 0;

export class ComponentPrototype {
  readonly id: string;
  readonly attributes: PropertyList | undefined;

  constructor(id: string, attributes?: PropertyList) {
    this.id = id;
    this.attributes = attributes;
  }

  static newId(): string {
    return '__component' + ++idSequence;
  }

  static instanceWithNewId(attributes?: PropertyList): ComponentPrototype {
    return new ComponentPrototype(ComponentPrototype.newId(), attributes);
  }
}
