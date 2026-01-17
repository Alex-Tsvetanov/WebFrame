export type DurationMills = number;
export type DurationMicros = number;

export class Duration {
  readonly milliseconds: DurationMills;

  constructor(milliseconds: DurationMills) {
    this.milliseconds = milliseconds;
  }

  get seconds(): number {
    return this.milliseconds / 1000.0;
  }

  get microseconds(): number {
    return this.milliseconds * 1000.0;
  }

  subtracting(other: Duration): Duration {
    return new Duration(this.milliseconds - other.milliseconds);
  }

  adding(other: Duration): Duration {
    return new Duration(this.milliseconds + other.milliseconds);
  }

  toString(): string {
    let current = this.microseconds;
    let unit = 'us';
    let precision = 0;
    if (current > 1000) {
      current /= 1000;
      unit = 'ms';
      precision = 2;
    }
    if (current > 1000) {
      current /= 1000;
      unit = 's';
      precision = 3;
    }

    return `${current.toFixed(precision)}${unit}`;
  }
}

export class Stopwatch {
  private startTime: DurationMills;
  private endTime: DurationMills;

  constructor() {
    this.startTime = 0;
    this.endTime = 0;
  }

  static createAndStart(): Stopwatch {
    const stopwatch = new Stopwatch();
    stopwatch.start();
    return stopwatch;
  }

  start() {
    this.startTime = performance.now();
    this.endTime = 0;
  }

  stop() {
    this.endTime = performance.now();
  }

  pause() {
    this.endTime = performance.now();
  }

  resume() {
    this.startTime = performance.now() - (this.endTime - this.startTime);
    this.endTime = 0;
  }

  elapsedMicros(): DurationMills {
    return this.elapsedMs() * 1000.0;
  }

  elapsedMs(): number {
    const endTime = this.endTime || performance.now();
    return endTime - this.startTime;
  }

  elapsed(): Duration {
    return new Duration(this.elapsedMs());
  }
}
