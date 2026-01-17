import { createInterface } from 'readline';
import { Command, Request, RequestBody, ResponseBody } from './protocol';
import { ZombieKiller } from './ZombieKiller';
import { ILogger } from './logger/ILogger';
import { StreamLogger } from './logger/StreamLogger';
import { getArgumentValue } from './utils/getArgumentValue';
import { sendError, sendResponse } from './utils/companionTransport';
import { ProtocolLogger } from './logger/ProtocolLogger';

// MARK: - Exit handling

export const enum ExitKind {
  GRACEFUL,
  UNGRACEFUL,
}

export class CompanionServiceBase {
  private zombieKiller: ZombieKiller | undefined;
  private requestHandlerByCommandName: { [name: string]: (body: any) => Promise<ResponseBody> } = {};

  protected logger: ILogger;

  constructor(readonly options: Set<string>) {
    if (options.has('--log-output')) {
      this.logger = StreamLogger.forOutputFile(getArgumentValue('--log-output'));
    } else if (options.has('--log-to-stderr')) {
      this.logger = StreamLogger.forStderr();
    } else if (options.has('--command')) {
      this.logger = StreamLogger.forStdOut();
    } else {
      this.logger = new ProtocolLogger();
    }
  }

  addEndpoint<Request, Response extends ResponseBody>(
    name: string,
    callback: (body: Request) => Promise<Response>,
  ): void {
    this.requestHandlerByCommandName[name] = callback;
  }

  start(): void {
    this.setupSignals();

    if (this.options.has('--command')) {
      const commandType = getArgumentValue('--command');

      if (!this.options.has('--body')) {
        throw new Error('--body should be provided when passing --command');
      }

      const bodyJson = getArgumentValue('--body');

      const body = JSON.parse(bodyJson);
      this.processCommand(commandType as Command, body as RequestBody).then((output) => {
        console.log(JSON.stringify(output, undefined, 2));
      });
    } else {
      this.logger.info?.('Started Compiler Companion');
      console.error('Node version:', process.version);
      this.zombieKiller = new ZombieKiller(() => {
        this.handleExit(ExitKind.UNGRACEFUL);
      });
      this.zombieKiller.start();

      const it = createInterface({
        input: process.stdin,
        output: process.stdout,
        terminal: false,
      });

      it.on('line', (line: string) => this.processLine(line));
      it.on('close', () => {
        console.error('Main interface close...');
        this.handleExit(ExitKind.GRACEFUL);
      });
    }
  }

  private async processCommand(command: string, body: RequestBody): Promise<ResponseBody> {
    const handler = this.requestHandlerByCommandName[command];
    if (!handler) {
      throw Error(`Unrecognized command '${command}'`);
    }

    return await handler(body);
  }

  private async processLine(l: string): Promise<void> {
    let requestId: string | undefined;
    try {
      const request: Request = JSON.parse(l);

      requestId = request.id;
      if (!requestId) {
        throw Error('Missing request id');
      }

      const response = await this.processCommand(request.command, request.body);

      sendResponse(requestId, response);
    } catch (err: any) {
      if (requestId) {
        this.logger.error?.(err);
        sendError(requestId, err);
      } else {
        console.error(`Unmatched error: ${err.message}`);
      }
    }
  }

  protected handleExit(exitKind: ExitKind): void {
    switch (exitKind) {
      case ExitKind.GRACEFUL:
        process.exitCode = 0;
      case ExitKind.UNGRACEFUL:
        process.exitCode = 1;
    }

    this.zombieKiller?.stop();
    process.stdin.unref();
  }

  private setupSignals(): void {
    const handleExitSignal = (signal: NodeJS.Signals, exitKind: ExitKind) => {
      console.error(`Companion received signal ${signal}, dying...`);
      this.handleExit(exitKind);
    };

    process.on('SIGINT', (signal) => {
      handleExitSignal(signal, ExitKind.UNGRACEFUL);
    });

    process.on('SIGTERM', (signal) => {
      handleExitSignal(signal, ExitKind.GRACEFUL);
    });
  }
}
