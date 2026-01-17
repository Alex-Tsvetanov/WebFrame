import * as Constants from '../core/constants';

export function wrapInColor(
  s: string | undefined,
  color: Constants.ANSI_COLORS,
  reset_color: Constants.ANSI_COLORS = Constants.ANSI_COLORS.RESET_COLOR,
): string {
  return `${color}${s ?? ''}${reset_color}`;
}
