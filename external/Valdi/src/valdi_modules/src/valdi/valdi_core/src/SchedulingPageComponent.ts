import 'valdi_tsx/src/JSX';
import { StatefulComponent } from './Component';
import { trace } from './utils/Trace';

/**
 * @see IRenderer.{strategy} for more information
 */
export const enum SchedulingStrategy {
  /**
   * Doesn't execute onCreateCritical at all
   */
  NoExecution = -1,

  /**
   * Called immediately upon placing in the view hierarchy after initial creation.
   *  This is the equivalent of the original Component, but this is only recommended for unit tests, or places
   *  where multiple strategies are required
   */
  Immediate = 0,

  /**
   * Called immediately after the current render ends, right after the render request has been sent to the runtime.
   *  You can expect this to happen immediately after the render calculation finished in the valdi thread,
   *  and will typically compete with the main thread for resources, but will happen sooner than LayoutComplete
   *
   * If you're looking to keep it as similar as the original onCreate while still splitting the work, this is the best option
   *
   * @see IRenderer.onRenderComplete
   */
  RenderComplete = 1,

  /**
   * Called after the layout pass has been completed, which is later than RenderComplete.
   *  You can expect this to happen 15-30ms after the render calculation finished in the valdi thread
   *
   * If you're looking for maximum deferral of non-critical work, while still guaranteeing execution in time, this is the best option
   *
   * @see IRenderer.onLayoutComplete
   */
  LayoutComplete = 2,
}

/**
 * Full page components can extend this class and differentiate between critical and non-critical processes
 *  to avoid delays on their first layout
 */
export abstract class SchedulingPageComponent<
  ViewModel = object,
  State = object,
  ComponentContext = object,
> extends StatefulComponent<ViewModel, State, ComponentContext> {
  /**
   * Equivalent to a normal's Component onCreate, executed immediately upon placing in the view hiearchy after initial
   *  creation, or after it has been destroyed. Blocks the initial render until it completes
   *
   * Suggested usage: prepare anything required for the first layout
   *   consider using onCreateNonCritical to subscribe to your non critical bridge observables instead
   */
  abstract onCreateCritical(): void;

  /**
   * Function that will be called after the critical initial render is complete
   *  depending on the scheduling strategy is exactly when it will happen
   *
   * Suggested usage: hydration of any non-critical data, or subscribing to bridge observables
   */
  abstract onCreateNonCritical(): void;

  /**
   * How should the non-critical onCreate be scheduled
   * @see SchedulingStrategy
   */
  schedulingStrategy: SchedulingStrategy = SchedulingStrategy.RenderComplete;

  private boundOnCreateCritical = this.onCreateCritical.bind(this);
  private boundOnCreateNonCritical = () => trace('onCreateNonCritical', () => this.onCreateNonCritical.call(this));

  // do not override this method, override onCreateCritical and onCreateNonCritical instead
  onCreate(): void {
    trace('onCreateCritical', () => {
      this.boundOnCreateCritical();
    });

    this.scheduleWithStrategy(this.boundOnCreateNonCritical);
  }

  /**
   * Schedule a callback with the current scheduling strategy
   * @param callback
   */
  scheduleWithStrategy(callback: () => void): void {
    if (this.schedulingStrategy === SchedulingStrategy.LayoutComplete) {
      this.renderer.onLayoutComplete(callback);
    } else if (this.schedulingStrategy === SchedulingStrategy.RenderComplete) {
      this.renderer.onRenderComplete(callback);
    } else if (this.schedulingStrategy === SchedulingStrategy.Immediate) {
      callback();
    } else {
      // NoExecution
    }
  }
}
