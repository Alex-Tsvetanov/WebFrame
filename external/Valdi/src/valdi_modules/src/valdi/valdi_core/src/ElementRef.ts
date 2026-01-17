import { StringMap } from 'coreutils/src/StringMap';
import { IRenderedElementBase } from 'valdi_tsx/src/IRenderedElementBase';
import { AttributesFrom, IRenderedElement } from './IRenderedElement';
import { IRenderedElementAttributeApplier } from './IRenderedElementApplier';

function compareElements<A, B>(left: IRenderedElement<A>, right: IRenderedElement<B>): number {
  return left.parentIndex - right.parentIndex;
}

/**
 * An ElementRef can be used to keep track of rendered elements in a tree.
 *
 */
export class ElementRef<T = any> implements IRenderedElementAttributeApplier<T> {
  // TODO: fix all current usages of new ElementRef and provide the right type parameter

  private elementById: { [id: number]: IRenderedElement<T> } = {};
  private elementByKey?: StringMap<IRenderedElement<T>>;
  private attributes?: AttributesFrom<T>;

  private _all?: IRenderedElement<T>[];
  all(): IRenderedElement<T>[] {
    let items = this._all;
    if (!items) {
      items = [];
      this._all = items;

      for (const elementId in this.elementById) {
        const element = this.elementById[elementId];
        if (element) {
          items.push(element);
        }
      }

      if (items.length > 1) {
        items.sort(compareElements);
      }
    }
    return items;
  }

  single(): IRenderedElement<T> | undefined {
    const items = this.all();
    if (!items.length) {
      return undefined;
    }
    if (items.length > 1) {
      throw Error(`Cannot get a single element from a ref which has ${items.length} elements`);
    }

    return items[0];
  }

  /**
   * Get the element for the key used during the rendering.
   * @param key
   */
  getForKey(key: string): IRenderedElement<T> | undefined {
    let elementByKey = this.elementByKey;
    if (!elementByKey) {
      // This is the first time we request an element given its key.
      // Construct the elementByKey dictionary so that subsequent
      // lookup are fast.
      elementByKey = {};
      this.elementByKey = elementByKey;

      for (const element of this.all()) {
        elementByKey[element.key] = element;
      }
    }

    return elementByKey[key];
  }

  /**
   * Set an attribute on all elements.
   * The attribute will be applied for any new elements which
   * get attached to this element ref.
   */
  setAttribute<K extends keyof AttributesFrom<T>>(attributeName: K, attributeValue: AttributesFrom<T>[K]): void {
    if (!this.attributes) {
      this.attributes = {};
    }

    this.attributes[attributeName] = attributeValue;

    for (const element of this.all()) {
      element.setAttribute(attributeName, attributeValue);
    }
  }

  /**
   * Get an attribute that was previously set using setAttribute()
   * @param attributeName
   */
  getAppliedAttribute(attributeName: keyof AttributesFrom<T>): any {
    if (!this.attributes) {
      return undefined;
    }
    return this.attributes[attributeName];
  }

  /**
   * Set all elements at once.
   * @param elements
   */
  setElements(elements: IRenderedElementBase<T>[]): void {
    // We check the items which are new.
    // We currently don't check for deletion. This means
    // that using the setElements API in conjunction with
    // the onElementCreated/Destroyed API will wield incorrect
    // results.

    for (const element of elements) {
      if (!this.elementById[(element as IRenderedElement<T>).id]) {
        this.onElementCreated(element);
      }
    }
    this._all = elements as IRenderedElement<T>[];
  }

  onElementCreated(element: IRenderedElementBase<T>): void {
    const typedElement = element as IRenderedElement<T>;
    this._all = undefined;
    this.elementById[typedElement.id] = typedElement;

    if (this.attributes) {
      for (const attributeName in this.attributes) {
        // TODO: I'm not sure why this type coercion is required at the moment
        const fixedAttributeName = attributeName as keyof AttributesFrom<T>;
        const attributeValue = this.attributes[fixedAttributeName];
        typedElement.setAttribute(fixedAttributeName, attributeValue);
      }
    }

    if (this.elementByKey) {
      this.elementByKey[typedElement.key] = typedElement;
    }
  }

  onElementDestroyed(element: IRenderedElementBase<T>): void {
    const typedElement = element as IRenderedElement<T>;
    this._all = undefined;
    delete this.elementById[typedElement.id];

    if (this.elementByKey) {
      delete this.elementByKey[typedElement.key];
    }
  }
}
