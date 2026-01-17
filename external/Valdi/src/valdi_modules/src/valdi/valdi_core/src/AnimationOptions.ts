export enum AnimationCurve {
  Linear = 'linear',
  EaseIn = 'easeIn',
  EaseOut = 'easeOut',
  EaseInOut = 'easeInOut',
}

export type AnimationOptions = SpringAnimationOptions | PresetCurveAnimationOptions | CustomCurveAnimationOptions;

export interface BasicAnimationOptions {
  /**
   * The duration of the animation in seconds.
   */
  duration: number;

  /**
   * Whether the animation should start from the current layer state.
   * Corresponds to CoreAnimation's beginFromCurrentState.
   */
  beginFromCurrentState?: boolean;

  /**
   * Whether the animation should crossfade with an alpha transition between the
   *  previous state of the tree to the new state.
   */
  crossfade?: boolean;

  /**
   * A completion to call when the transition has completed
   */
  completion?: (wasCancelled: boolean) => void;
}

export interface PresetCurveAnimationOptions extends BasicAnimationOptions {
  /**
   * The curve to use for the animation, defaults
   * to easeInOut. Ignored when controlPoints is set.
   */
  curve?: AnimationCurve;
}

export interface CustomCurveAnimationOptions extends BasicAnimationOptions {
  /**
   * 4 control points to control the curve.
   * If set, curve will be ignored to use those
   * control points instead.
   */
  controlPoints: Array<number>;
}

export interface SpringAnimationOptions {
  /**
   * The spring stiffness coefficient. Higher values correspond to a stiffer spring that yields a greater amount of force for moving objects.
   * Corresponds to the "spring constant" in the spring equation.
   *
   * Must be greater than 0.
   * See: https://drive.google.com/file/d/1z5OUi8hhO4ZdgR6hkHCbwy-BRLJG8mjs/view?usp=sharing
   */
  stiffness: SpringStiffnessValue;

  /**
   * The damping force to apply to the spring’s motion. This is the damping coefficient in the spring equation.
   *
   * Must be greater than 0.
   * See: https://drive.google.com/file/d/1z5OUi8hhO4ZdgR6hkHCbwy-BRLJG8mjs/view?usp=sharing
   */
  damping: SpringDampingValue;

  /**
   * Whether the animation should start from the current layer state.
   * Corresponds to CoreAnimation's beginFromCurrentState.
   */
  beginFromCurrentState?: boolean;

  /**
   * A completion to call when the transition has completed
   */
  completion?: (wasCanceled: boolean) => void;
}

// (Taken from Principle.app)
export const SPRING_DEFAULT_STIFFNESS: SpringStiffnessValue = 381.47;
// (Taken from Principle.app)
export const SPRING_DEFAULT_DAMPING: SpringDampingValue = 20.1;

export type SpringStiffnessValue = number;
export type SpringDampingValue = number;
