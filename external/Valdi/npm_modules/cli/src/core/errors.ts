import { ANSI_COLORS } from './constants';

export class CliError extends Error {
  textColor: ANSI_COLORS;

  constructor(message: string, textColor: ANSI_COLORS = ANSI_COLORS.RED_COLOR) {
    super(message);
    this.textColor = textColor;
  }
}
