export enum TimeUnit {
  HOURS,
  SECONDS,
}

export function convertTimestampMs(timestampMs: number, unit: TimeUnit): number {
  switch (unit) {
    case TimeUnit.HOURS:
      return timestampMs / 1000 / 60 / 60;
    case TimeUnit.SECONDS:
      return timestampMs / 1000;
  }
}
