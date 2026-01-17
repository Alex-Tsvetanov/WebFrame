import { ILogger } from './ILogger';
import * as fs from 'fs';
import { formatMessages } from './LoggerUtils';

export class StreamLogger implements ILogger {
  constructor(readonly stream: NodeJS.WritableStream) {}

  debug = (...messages: any[]) => {
    this.enqueueLog('DEBUG', messages);
  };

  info = (...messages: any[]) => {
    this.enqueueLog('INFO', messages);
  };

  warn = (...messages: any[]) => {
    this.enqueueLog('WARN', messages);
  };

  error = (...messages: any[]) => {
    this.enqueueLog('ERROR', messages);
  };

  private enqueueLog(type: string, messages: any[]): void {
    const logDate = new Date();
    const formatted = formatMessages(messages);
    this.stream.write(`${logDate} [${type}]: ${formatted}\n`);
  }

  static forOutputFile(outputPath: string): StreamLogger {
    return new StreamLogger(fs.createWriteStream(outputPath));
  }

  static forStderr(): StreamLogger {
    return new StreamLogger(process.stderr);
  }

  static forStdOut(): StreamLogger {
    return new StreamLogger(process.stdout);
  }
}
