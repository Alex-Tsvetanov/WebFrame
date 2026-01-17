export const enum DebugLevel {
  DEBUG = 0,
  INFO,
  WARN,
  ERROR,
}

export type SubmitDebugMessageFunc = (level: DebugLevel, message: string) => void;
