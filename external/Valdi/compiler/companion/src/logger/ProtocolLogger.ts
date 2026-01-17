import { sendEvent } from '../utils/companionTransport';
import { ILogger, ILoggerStream } from './ILogger';
import { formatMessages } from './LoggerUtils';

export class ProtocolLogger implements ILogger {
  debug: ILoggerStream | undefined = undefined;
  info = (...messages: any[]) => {
    this.writeLogs('info', messages);
  };

  warn = (...messages: any[]) => {
    this.writeLogs('warn', messages);
  };

  error = (...messages: any[]) => {
    this.writeLogs('error', messages);
  };

  private writeLogs(type: string, messages: any[]): void {
    sendEvent(type, formatMessages(messages));
  }
}
