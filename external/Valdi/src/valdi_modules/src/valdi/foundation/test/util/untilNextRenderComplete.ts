import { IComponent } from 'valdi_core/src/IComponent';

export function untilNextRenderComplete(component: IComponent): Promise<void> {
  return new Promise(resolve => {
    component.renderer.onNextRenderComplete(resolve);
  });
}
