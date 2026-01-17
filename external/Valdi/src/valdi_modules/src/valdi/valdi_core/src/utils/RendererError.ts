import { IRenderedVirtualNode } from '../IRenderedVirtualNode';

export class RendererError extends Error {
  constructor(
    message: string,
    readonly sourceError: Error,
    readonly lastRenderedNode: IRenderedVirtualNode | undefined,
  ) {
    super(message);
  }
}
