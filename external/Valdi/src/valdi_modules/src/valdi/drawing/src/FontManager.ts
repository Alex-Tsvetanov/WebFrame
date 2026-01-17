import { IFontProvider } from 'valdi_tsx/src/IFontProvider';
import { FontStyle, FontWeight } from './DrawingModuleProvider';
import {
  IFontManagerNative,
  getDefaultFontManager,
  makeScopedFontManager,
  registerFontFromData,
  registerFontFromFilePath,
} from './FontManagerNative';

let cachedDefault: FontManager | undefined = undefined;

export class FontManager {
  private constructor(readonly native: IFontManagerNative) {}

  /**
   * Return the IFontProvider instance suitable to pass as the `fontProvider`
   * attribute of the lottie element.
   */
  get fontProvider(): IFontProvider {
    return this.native;
  }

  /**
   * Returns the default font manager associated with the runtime
   */
  static getDefault(): FontManager {
    if (!cachedDefault) {
      cachedDefault = new FontManager(getDefaultFontManager());
    }
    return cachedDefault;
  }

  /**
   * Creates a scoped font manager from the given font manager instance.
   * Any registered fonts within the returned scoped font manager will impact
   * the scoped font manager only. The returned instance will inherit the fonts
   * from its parent font manager
   */
  makeScoped(): FontManager {
    return new FontManager(makeScopedFontManager(this.native));
  }

  /**
   * Registers a font into the font manager from bytes representing the font data
   */
  registerFontFromData(fontName: string, weight: FontWeight, style: FontStyle, fontData: Uint8Array): void {
    registerFontFromData(this.native, fontName, weight, style, fontData);
  }

  /**
   * Registers a font into the font manager from bytes from a file path
   */
  registerFontFromFilePath(fontName: string, weight: FontWeight, style: FontStyle, filePath: string): void {
    registerFontFromFilePath(this.native, fontName, weight, style, filePath);
  }
}
