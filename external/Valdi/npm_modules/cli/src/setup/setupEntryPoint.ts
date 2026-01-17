import { ANSI_COLORS } from '../core/constants';
import { wrapInColor } from '../utils/logUtils';
import { linuxSetup } from './linuxSetup';
import { macOSSetup } from './macOSSetup';

enum OSType {
  MACOS = 'macos',
  WINDOWS = 'windows',
  LINUX = 'linux',
  UNKNOWN = 'unknown',
}

function getOSType(): OSType {
  switch (process.platform) {
    case 'darwin': {
      return OSType.MACOS;
    }
    case 'win32': {
      return OSType.WINDOWS;
    }
    case 'linux': {
      return OSType.LINUX;
    }
    default: {
      return OSType.UNKNOWN;
    }
  }
}

export async function beginEnvironmentSetup(): Promise<number> {
  const os = getOSType();
  const returnCode = 0;

  switch (os) {
    case OSType.MACOS: {
      console.log(wrapInColor('Setting up development environment for MacOS...', ANSI_COLORS.BLUE_COLOR));
      await macOSSetup();
      break;
    }
    case OSType.WINDOWS: {
      console.log(wrapInColor('Windows is not a supported operating system...aborting...', ANSI_COLORS.RED_COLOR));
      return 1;
    }
    case OSType.LINUX: {
      console.log(wrapInColor('Setting up development environment for Linux...', ANSI_COLORS.BLUE_COLOR));
      await linuxSetup();
      break;
    }
    default: {
      console.log(
        wrapInColor(
          'Could not determine current operating system, skipping automated environment setup...',
          ANSI_COLORS.RED_COLOR,
        ),
      );
      console.log(
        'Please see https://github.com/Snapchat/Valdi/blob/main/docs/INSTALL.md for manual setup instructions.',
      );
      return 1;
    }
  }

  return returnCode;
}
