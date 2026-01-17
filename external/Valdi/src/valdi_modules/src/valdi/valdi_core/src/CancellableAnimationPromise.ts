import { AnimationOptions } from './AnimationOptions';
import { CancellableAnimation, CancellableAnimationState } from './CancellableAnimation';
import { IRenderer } from './IRenderer';

export class CancellableAnimationPromise implements CancellableAnimation {
  private renderer: IRenderer;
  private options: AnimationOptions;
  private animations: () => void;
  private cancelToken?: number;
  private cancelled?: boolean;
  private started?: boolean;

  constructor(renderer: IRenderer, options: AnimationOptions, animations: () => void) {
    this.renderer = renderer;
    this.options = options;
    this.animations = animations;
  }

  start(): Promise<boolean> {
    if (this.started) {
      throw new Error('Tried to start Animation twice');
    }
    this.started = true;
    return new Promise(resolve => {
      this.cancelToken = this.renderer.animate(
        {
          ...this.options,
          completion: cancelled => {
            this.options.completion?.(cancelled);
            this.cancelled = cancelled;
            resolve(cancelled);
          },
        },
        this.animations,
      );
    });
  }

  cancel(): void {
    if (this.cancelToken !== undefined) {
      this.renderer.cancelAnimation(this.cancelToken);
      this.cancelToken = undefined;
    }
  }

  getState(): CancellableAnimationState {
    if (this.started) {
      return CancellableAnimationState.Running;
    }
    if (this.cancelled === true) {
      return CancellableAnimationState.Cancelled;
    }
    return CancellableAnimationState.Finished;
  }
}
