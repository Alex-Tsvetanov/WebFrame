import { IComponent } from '../IComponent';

export interface IDetachedSlotRenderer extends IComponent {}

/**
 * a DetachedSlot allows to render elements and components into
 * a slot hosted by a component in an arbitrary location in the tree.
 * A DetachedSlot allows to disassociate the elements being rendered,
 * and where those elements are rendered. After a DetachedSlot is created,
 * a DetachedSlotRenderer should be created and rendered in the tree.
 * The DetachedSlotRenderer will render the slot in the subtree
 * where it is rendered. A separate component could then use the slot to set
 * the render function that should be evaluated by the DetachedSlotRenderer.
 */
export class DetachedSlot {
  private renderers: IDetachedSlotRenderer[] = [];
  private renderFn?: () => void;

  /**
   * Sets the render function that should be rendered inside the DetachedSlotRenderer.
   * If no DetachedSlotRenderer were created, this call will be a no-op until a
   * DetachedSlotRenderer is created with this Zone.
   * @param fn the render function to evaluate inside the DetachedSlotRenderer.
   */
  slotted(fn: () => void) {
    this.renderFn = fn;

    for (const renderer of this.renderers) {
      renderer.renderer.renderComponent(renderer, undefined);
    }
  }

  attachRenderer(renderer: IDetachedSlotRenderer): void {
    this.renderers.push(renderer);
  }

  detachRenderer(renderer: IDetachedSlotRenderer): void {
    const index = this.renderers.indexOf(renderer);
    if (index >= 0) {
      this.renderers.splice(index, 1);
    }
  }

  onRendererReady(renderer: IDetachedSlotRenderer) {
    if (this.renderFn) {
      this.renderFn();
    }
  }
}
