import { ANSI_COLORS } from '../core/constants';
import { beginEnvironmentSetup } from '../setup/setupEntryPoint';
import { makeCommandHandler } from '../utils/errorUtils';
import { wrapInColor } from '../utils/logUtils';

async function valdiSetup() {
  const returnCode = await beginEnvironmentSetup();

  if (returnCode !== 0) {
    console.log(wrapInColor('Environment setup did not complete successfully.', ANSI_COLORS.YELLOW_COLOR));
  }
}

export const command = 'dev_setup';
export const describe = 'Sets up the development environment for Valdi';
export const builder = () => {};
export const handler = makeCommandHandler(valdiSetup);
