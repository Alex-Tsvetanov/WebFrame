import { IComponent } from 'valdi_core/src/IComponent';

export function untilRenderComplete(component: IComponent): Promise<void> {
  return new Promise(resolve => {
    component.renderer.onRenderComplete(resolve);
  });
}
