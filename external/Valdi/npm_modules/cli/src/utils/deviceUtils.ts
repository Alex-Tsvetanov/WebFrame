import { Architecture, PLATFORM } from '../core/constants';
import { CliError } from '../core/errors';
import { runCliCommand, spawnCliCommand } from './cliUtils';

function parseAndroidDevices(devicesString: string): string[] {
  const deviceIDs: string[] = devicesString
    .split('\n')
    .filter(line => line.trim().endsWith('device') && !line.startsWith('List of devices'))
    .map(line => line.split('\t')?.[0]?.trim() ?? ''); // Extract the device ID

  return deviceIDs ?? [];
}

export async function getArchitectureForAndroidDevice(deviceId: string): Promise<Architecture> {
  const { stdout } = await runCliCommand(`adb -s ${deviceId} shell getprop ro.product.cpu.abi`, undefined, true);

  const architectureString = stdout.trim();

  switch (architectureString) {
    case 'arm64-v8a': {
      return Architecture.ARM64;
    }
    case 'armeabi-v7a': {
      return Architecture.ARMV7;
    }
    case 'armeabi': {
      return Architecture.ARMV7;
    }
    case 'x86_64': {
      return Architecture.X86_64;
    }
    default: {
      throw new CliError(`Unsupported device architecture: ${architectureString}`);
    }
  }
}

export function getAllArchitecturesForAndroid() {
  return [Architecture.ARM64, Architecture.ARMV7, Architecture.X86_64];
}

export async function getConnectedAndroidDevices(): Promise<string[]> {
  const commandListDevices = 'adb devices';
  const { stdout: devicesString } = await runCliCommand(commandListDevices, undefined, true);
  const androidDevices = parseAndroidDevices(devicesString);

  return androidDevices;
}

export async function getConnectedIOSDevices(): Promise<string[]> {
  // eslint-disable-next-line unicorn/no-useless-promise-resolve-reject
  return Promise.resolve(['-1']);
}

export async function getConnectedDevices(platform: string): Promise<string[]> {
  if (platform === PLATFORM.ANDROID) {
    const androidDevices = await getConnectedAndroidDevices();
    return androidDevices;
  } else if (platform === PLATFORM.IOS) {
    const iosDevices = await getConnectedIOSDevices();
    return iosDevices;
  }

  return [];
}

export async function installAndroidApk(apkPath: string, emulatorId?: string) {
  const idString = emulatorId ? ` -s ${emulatorId} ` : ' ';
  const commandInstallApk = `adb${idString}install -r ${apkPath}`;
  await spawnCliCommand(commandInstallApk, undefined, 'pipe', false, true);
}

export async function startAndroidActivity(activityName: string) {
  const commandStartActivity = `adb shell am start -n ${activityName}`;
  await spawnCliCommand(commandStartActivity, undefined, 'pipe', false, true);
}
