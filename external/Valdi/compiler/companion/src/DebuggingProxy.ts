import { HermesDevice } from 'remotedebug-ios-webkit-adapter/src/adapters/hermesAdapter';
import { ProxyServer } from 'remotedebug-ios-webkit-adapter/src/server';
import { ILogger } from './logger/ILogger';
import { rethrow } from './utils/rethrow';

const port = 9010;

type AndroidDevice = { deviceId: string; endpoint: string };

export class DebuggingProxy {
  private knownAndroidDevices: AndroidDevice[] = [];
  private knownHermesDevices: HermesDevice[] = [];
  private server: ProxyServer | undefined;

  constructor(private logger: ILogger | undefined) {}

  async start() {
    if (this.server) {
      throw new Error("Cannot start debugging proxy while it's already started");
    }
    this.server = new ProxyServer();
    try {
      const actualPort = await this.server.run(port);
      this.logger?.debug?.(`Valdi debugging proxy is listening on port ${actualPort}`);
      this.server.updateKnownAndroidDevices(this.knownAndroidDevices);
      this.server.updateKnownHermesDevices(this.knownHermesDevices);

      return actualPort;
    } catch (err: any) {
      this.server = undefined;
      let message = `
⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️
⚠️ Valdi debugging proxy failed to start with error:
⚠️ ${err.message || err}
⚠️
⚠️ 'Valdi Attach' debug configuration in VS Code will not work.
⚠️ To avoid this error, re-run the hotreloader script with --no-debugging-proxy:
⚠️
⚠️ Consider using 'Hermes Attach' debug configuration in VS Code for a more stable
⚠️ debugging environment. Additional info at:
⚠️ https://github.com/Snapchat/Valdi/blob/main/docs/docs/workflow-hermes-debugger.md
⚠️
⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️
`;
      rethrow(message, err);
    }
  }

  stop() {
    this.server?.stop();
    this.server = undefined;
  }

  updateAvailableAndroidDevices(androidDevices: AndroidDevice[]) {
    this.knownAndroidDevices = androidDevices;
    this.server?.updateKnownAndroidDevices(androidDevices);
  }
  updateAvailableHermesDevices(hermesDevices: HermesDevice[]) {
    this.knownHermesDevices = hermesDevices;
    this.server?.updateKnownHermesDevices(hermesDevices);
  }
}
