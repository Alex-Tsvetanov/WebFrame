import { ElementFrame } from 'valdi_tsx/src/Geometry';
import { Point, Size } from '../Geometry';
import { IRenderedElement } from '../IRenderedElement';

export namespace RenderedElementUtils {
  /**
   * Compute a relative position within the element tree by recursively looking through the parents
   */
  export function relativePositionTo(parent: IRenderedElement, child: IRenderedElement): Point | undefined {
    let current: IRenderedElement | undefined = child;
    const position = { x: 0, y: 0 };
    while (current) {
      if (current == parent) {
        return position;
      }
      const contentOffsetX = current.getAttribute('contentOffsetX') ?? 0;
      const contentOffsetY = current.getAttribute('contentOffsetY') ?? 0;
      const translationX = current.getAttribute('translationX') ?? 0;
      const translationY = current.getAttribute('translationY') ?? 0;
      position.x += current.frame.x - contentOffsetX + translationX;
      position.y += current.frame.y - contentOffsetY + translationY;
      current = current.parent;
    }
    return undefined;
  }
  /**
   * Check if a relative position is within the bounds of a frame
   */
  export function frameContainsPosition(frame: ElementFrame, position: Point): boolean {
    if (position.x > frame.width || position.y > frame.height) {
      return false;
    }
    if (position.x < 0 || position.y < 0) {
      return false;
    }
    return true;
  }
  /**
   * Extract the origin of an element frame
   */
  export function framePosition(frame: ElementFrame): Point {
    return frame;
  }
  /**
   * Extract the size of an element frame
   */
  export function frameSize(frame: ElementFrame): Size {
    return frame;
  }
}
