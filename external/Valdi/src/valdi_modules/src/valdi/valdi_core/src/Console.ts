enum ConsoleLevel {
  Debug = 0,
  Log = 1,
  Info = 2,
  Warn = 3,
  Error = 4,
}

import { debugStringify } from './utils/StringUtils';

export class Console {
  constructor(private readonly logFunction: (type: number, content: string) => void) {}

  private submit(level: ConsoleLevel, args: any[]) {
    const results: string[] = ['[JS]'];
    for (const arg of args) {
      results.push(debugStringify(arg, 20, true));
    }
    this.logFunction(level, results.join(' '));
  }

  debug(...args: any[]) {
    this.submit(ConsoleLevel.Debug, args);
  }
  log(...args: any[]) {
    this.submit(ConsoleLevel.Log, args);
  }
  info(...args: any[]) {
    this.submit(ConsoleLevel.Info, args);
  }
  warn(...args: any[]) {
    this.submit(ConsoleLevel.Warn, args);
  }
  error(...args: any[]) {
    this.submit(ConsoleLevel.Error, args);
  }
}
