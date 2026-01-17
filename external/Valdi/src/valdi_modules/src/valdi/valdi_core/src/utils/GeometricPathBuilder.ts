import { GeometricPath } from 'valdi_tsx/src/GeometricPath';

export type { GeometricPath };

const enum GeometricPathComponent {
  Move = 1,
  Line = 2,
  Quad = 3,
  Cubic = 4,
  RoundRect = 5,
  Arc = 6,
  Close = 7,
}

/**
 * Defines how the GeometricPath should scale relative to
 * the container view where it is being used.
 */
export const enum GeometricPathScaleType {
  /**
   * The path will scale such that the path will entirely
   * fill the bounds of the container without expanding beyond it.
   * Aspect ratio of the path will not be preserved.
   * This is the default option.
   */
  Fill = 1,

  /**
   * The path will scale such that the path will fit within the bounds of the
   * container. Aspect ratio of the path will be preserved.
   */
  Contain = 2,

  /**
   * The path will scale such that the path entirely fill the bounds of
   * the container, with sides potentially expanding beyond it.
   * Aspect ratio of the path will be preserved.
   */
  Cover = 3,

  /**
   * The path will not be scaled relative to the container. In this mode,
   * the extentWidth and extentHeight will be treated as the absolute size
   * of the path, and the path will be centered in the container.
   */
  None = 4,
}

/**
 * A GeometricPathBuilder is a utility class for building
 * GeometricPath objects that can be provided to the "shape" elements.
 * A path builder is created with an extent width and height, which represent
 * the coordinate space from which all the path operation values derive from.
 * Whenever the path is used into a container that has a different size than the
 * path extent, the path operations are scaled by applying a ratio between
 * the extent and the actual container size.
 */
export class GeometricPathBuilder {
  private data: number[] = [];

  constructor(extentWidth: number, extentHeight: number, scaleType?: GeometricPathScaleType) {
    this.data.push(extentWidth, extentHeight, scaleType ?? GeometricPathScaleType.Fill);
  }

  /**
   * Move the path position to given x and y.
   */
  moveTo(x: number, y: number): GeometricPathBuilder {
    this.data.push(GeometricPathComponent.Move, x, y);
    return this;
  }

  /**
   * Add a line from the current path position to the given x and y
   */
  lineTo(x: number, y: number): GeometricPathBuilder {
    this.data.push(GeometricPathComponent.Line, x, y);
    return this;
  }

  /**
   * Add a rect at the given x and y with the given width and height.
   */
  rectTo(x: number, y: number, width: number, height: number): GeometricPathBuilder {
    return this.roundRectTo(x, y, width, height, 0, 0);
  }

  /**
   * Add a round rect at the given x and y with the given width and height, with
   * the rect corners rounded using radiusX and radiusY.
   */
  roundRectTo(
    x: number,
    y: number,
    width: number,
    height: number,
    radiusX: number,
    radiusY: number,
  ): GeometricPathBuilder {
    this.data.push(GeometricPathComponent.RoundRect, x, y, width, height, radiusX, radiusY);
    return this;
  }

  /**
   * Add a arc at the given centerX and centerY with the given radius, starting from
   * the given startAngle and sweeping by the given sweepAngle.
   * A positive sweepAngle extends the arc clockwise, a negative angle extends
   * it counterclockwise.
   */
  arcTo(
    centerX: number,
    centerY: number,
    radius: number,
    startAngle: number,
    sweepAngle: number,
  ): GeometricPathBuilder {
    this.data.push(GeometricPathComponent.Arc, centerX, centerY, radius, startAngle, sweepAngle);
    return this;
  }

  /**
   * Add an oval at the given x and y with the given width and height.
   */
  ovalTo(x: number, y: number, width: number, height: number): GeometricPathBuilder {
    return this.roundRectTo(x, y, width, height, width / 2.0, height / 2.0);
  }

  /**
   * Add a cubic curve from the current path position to the given x and y,
   * using controlX1 and controlY1 as the first control point of the curve,
   * and controlX2 and controlY2 as the second control point of the curve.
   */
  cubicTo(
    x: number,
    y: number,
    controlX1: number,
    controlY1: number,
    controlX2: number,
    controlY2: number,
  ): GeometricPathBuilder {
    this.data.push(GeometricPathComponent.Cubic, x, y, controlX1, controlY1, controlX2, controlY2);
    return this;
  }

  /**
   * Add a quad curve from the current path position to the given x and y,
   * using controlX and controlY as the control point.
   */
  quadTo(x: number, y: number, controlX: number, controlY: number): GeometricPathBuilder {
    this.data.push(GeometricPathComponent.Quad, x, y, controlX, controlY);
    return this;
  }

  /**
   * Close the current path contour, connecting a line between the first point and the last point.
   */
  close(): GeometricPathBuilder {
    this.data.push(GeometricPathComponent.Close);
    return this;
  }

  /**
   * Build and return a GeometricPath object containing the path instructions.
   * The object can be then passed as to the "path" property of "shape" element.
   */
  build(): GeometricPath {
    return Float64Array.from(this.data);
  }
}
