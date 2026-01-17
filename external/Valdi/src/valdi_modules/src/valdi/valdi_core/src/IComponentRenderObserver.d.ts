import { IComponent } from './IComponent';

export interface IComponentRenderObserver {
  /**
   * Called whenever one of the component starts off a new rendering pass.
   * Note that this called just when a component initiates a completely new
   * render, it is not called for every components being rendered as part
   * of a single render pass.
   *
   * @param component the component that initiated the rendering.
   */
  onComponentWillRerender(component: IComponent): void;
}
