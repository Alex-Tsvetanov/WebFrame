import { Renderer } from './Renderer';

export interface RendererFactory {
  makeRenderer(treeId: string): Renderer;
}
