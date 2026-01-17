import { IRenderer } from 'valdi_core/src/IRenderer';
import { Size } from './DrawingModuleProvider';
import { IBitmap } from './IBitmap';
import type { Rect } from './ManagedContextNative';

export type { Rect };

export const enum MeasureMode {
  UNSPECIFIED = 0,
  EXACTLY = 1,
  AT_MOST = 2,
}

export interface RasterResult {
  damageRects: Rect[];
}

export interface IManagedContextAssetsLoadResult {
  loadedAssetsCount: number;
  errors: string[] | undefined;
}

export interface IManagedContext {
  /**
   * Return the underlying Renderer managed by this ManagedContext
   */
  renderer: IRenderer;

  /**
   * Starts a render and evaluate the given render function
   */
  render(renderFunc: () => void): void;

  /**
   * Measure the context with the given max size and returned
   * a size that fits within the size constraint.
   */
  measure(maxWidth: number, widthMode: MeasureMode, maxHeight: number, heightMode: MeasureMode, rtl: boolean): Size;

  /**
   * Set the layout specifications of the tree
   */
  layout(width: number, height: number, rtl: boolean): void;

  /**
   * Draw the rendered tree with the previously provided layout specs.
   * Return a frame that contains the instructions that represents the scene.
   * The frame can be then rasterized either at the TS level or at the native level,
   * in any thread.
   */
  draw(): IManagedContextFrame;

  /**
   * Schedule a callback to be called when all the assets have been loaded.
   */
  onAllAssetsLoaded(): Promise<IManagedContextAssetsLoadResult>;

  /**
   * Destroy the managed context and its associated resources
   */
  dispose(): void;
}

export interface IManagedContextFrame {
  /**
   * Immediately dispose the frame and its associated resources
   */
  dispose(): void;

  /**
   * Rasterize the frame into a bitmap provided as an array buffer.
   */
  rasterInto(bitmap: IBitmap, shouldClearBitmapBeforeDrawing: boolean): RasterResult;

  /**
   * Rasterize the frame into a bitmap provided as an array buffer.
   * Only the areas that have changed since the last rasterization will be rasterized.
   */
  rasterDeltaInto(bitmap: IBitmap): RasterResult;
}
