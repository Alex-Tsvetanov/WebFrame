export interface ComponentCtor {
  name: string;
}

/**
 * The RendererEventListener can be set on a IRenderer instance
 * to be notified about important render events that occurs within the renderer.
 * The implementation of the event listener should be as lightweight as possible
 * to reduce performance overhead while rendering.
 */
export interface IRendererEventListener {
  onRenderBegin(): void;
  onRenderEnd(): void;

  onComponentBegin(key: string, componentCtor: ComponentCtor): void;
  onComponentEnd(): void;
  onBypassComponentRender(): void;
  onComponentViewModelPropertyChange(viewModelPropertyName: string): void;
}
