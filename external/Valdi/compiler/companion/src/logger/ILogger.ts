export type ILoggerStream = (...messages: any[]) => void;

export interface ILogger {
  debug: ILoggerStream | undefined;
  info: ILoggerStream | undefined;
  warn: ILoggerStream | undefined;
  error: ILoggerStream | undefined;
}
