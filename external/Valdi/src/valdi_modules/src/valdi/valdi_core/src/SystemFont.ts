export const SYSTEM_FONT_NAME = 'system';
export const SYSTEM_BOLD_FONT_NAME = 'system-bold';

function makeFont(fontName: string, size: number): string {
  return `${fontName} ${size}`;
}

/**
 * Returns a font name for the system font
 * with the given size. The return value is suitable
 * for use as a "font" attribute of <label>, <textview>
 * or <textfield> elements.
 */
export function systemFont(size: number): string {
  return makeFont(SYSTEM_FONT_NAME, size);
}

/**
 * Returns a font name for the bold system font
 * with the given size. The return value is suitable
 * for use as a "font" attribute of <label>, <textview>
 * or <textfield> elements.
 */
export function systemBoldFont(size: number): string {
  return makeFont(SYSTEM_BOLD_FONT_NAME, size);
}
