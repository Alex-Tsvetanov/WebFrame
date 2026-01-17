/**
 * Clamps the given value between the given minimum float and maximum float values.
 * Returns the given value if it is within the min and max range.
 */
export function clamp(value: number, min: number, max: number): number {
  if (max < min) {
    max = min;
  }
  return Math.max(Math.min(value, max), min);
}

/**
 * Linearly interpolates between start and end by ratio.
 * When ratio = 0 returns start.
 * When ratio = 1 return end.
 * When ratio = 0.5 returns the midpoint of start and end.
 */
export function lerp(start: number, end: number, ratio: number): number {
  return start * (1 - ratio) + end * ratio;
}
