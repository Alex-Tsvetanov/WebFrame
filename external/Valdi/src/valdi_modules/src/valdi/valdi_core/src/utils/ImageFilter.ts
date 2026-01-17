import { ImageFilter } from 'valdi_tsx/src/ImageFilter';

export type { ImageFilter };

const enum FilterType {
  Blur = 1,
  ColorMatrix = 2,
}

function clampRatio(ratio: number): number {
  return Math.min(Math.max(ratio, 0), 1.0);
}

export namespace ImageFilters {
  /**
   * Applies a blur filter to the image
   * @param radius
   * @returns
   */
  export function blur(radius: number): ImageFilter {
    return [FilterType.Blur, radius];
  }

  /**
   * Applies a color matrix filter to the image
   * @returns
   */
  export function colorMatrix(
    red1: number,
    red2: number,
    red3: number,
    red4: number,
    redBias: number,
    green1: number,
    green2: number,
    green3: number,
    green4: number,
    greenBias: number,
    blue1: number,
    blue2: number,
    blue3: number,
    blue4: number,
    blueBias: number,
    alpha1: number,
    alpha2: number,
    alpha3: number,
    alpha4: number,
    alphaBias: number,
  ): ImageFilter {
    return [
      FilterType.ColorMatrix,
      red1,
      red2,
      red3,
      red4,
      redBias,
      green1,
      green2,
      green3,
      green4,
      greenBias,
      blue1,
      blue2,
      blue3,
      blue4,
      blueBias,
      alpha1,
      alpha2,
      alpha3,
      alpha4,
      alphaBias,
    ];
  }

  /**
   * Invert the image colors
   */
  export function invert(): ImageFilter {
    return colorMatrix(-1, 0, 0, 0, 1, 0, -1, 0, 0, 1, 0, 0, -1, 0, 1, 0, 0, 0, 1, 0);
  }

  /**
   * Applies a grayscale filter to the image
   * @param intensity the intensity of the grayscale effect between 0 to 1
   * @returns
   */
  export function grayscale(intensity: number = 1): ImageFilter {
    const cv = 1.0 - clampRatio(intensity);

    return colorMatrix(
      0.2126 + 0.7874 * cv,
      0.7152 - 0.7152 * cv,
      0.0722 - 0.0722 * cv,
      0,
      0,
      0.2126 - 0.2126 * cv,
      0.7152 + 0.2848 * cv,
      0.0722 - 0.0722 * cv,
      0,
      0,
      0.2126 - 0.2126 * cv,
      0.7152 - 0.7152 * cv,
      0.0722 + 0.9278 * cv,
      0,
      0,
      0,
      0,
      0,
      1,
      0,
    );
  }

  /**
   * Applies a sepia filter to the image
   * @param intensity the intensity of the sepia effect between 0 to 1
   * @returns
   */
  export function sepia(intensity: number = 1) {
    const cv = 1.0 - clampRatio(intensity);

    return colorMatrix(
      0.393 + 0.607 * cv,
      0.769 - 0.769 * cv,
      0.189 - 0.189 * cv,
      0,
      0,
      0.349 - 0.349 * cv,
      0.686 + 0.314 * cv,
      0.168 - 0.168 * cv,
      0,
      0,
      0.272 - 0.272 * cv,
      0.534 - 0.534 * cv,
      0.131 + 0.869 * cv,
      0,
      0,
      0,
      0,
      0,
      1,
      0,
    );
  }

  /**
   * Adjusts the brightness of the image
   * @param value
   * @returns
   */
  export function brightness(value: number) {
    return colorMatrix(value, 0, 0, 0, 0, 0, value, 0, 0, 0, 0, 0, value, 0, 0, 0, 0, 0, 1, 0);
  }

  /**
   * Adjusts the contrast of the image
   * @param value
   * @returns
   */
  export function contrast(value: number) {
    const n = 0.5 * (1 - value);

    return colorMatrix(value, 0, 0, 0, n, 0, value, 0, 0, n, 0, 0, value, 0, n, 0, 0, 0, 1, 0);
  }

  /**
   * Compose a list of filters into a new filter
   * @param filters
   * @returns a new filter containing all the given filters
   */
  export function compose(...filters: ImageFilter[]): ImageFilter {
    const out: ImageFilter = [];
    for (const filter of filters) {
      out.push(...filter);
    }
    return out;
  }
}
