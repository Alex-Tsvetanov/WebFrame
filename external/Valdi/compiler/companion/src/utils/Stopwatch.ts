export class Stopwatch {
  private timeAtStart: number = 0;

  get elapsedSeconds(): number {
    return this.elapsedMilliseconds / 1000.0;
  }

  get elapsedMilliseconds(): number {
    return performance.now() - this.timeAtStart;
  }

  get elapsedString(): string {
    let current = this.elapsedMilliseconds;
    let unit = 'ms';
    let precision = 2;
    if (current > 1000) {
      current /= 1000;
      unit = 's';
      precision = 3;
    }

    return `${current.toFixed(precision)}${unit}`;
  }

  constructor() {
    this.start();
  }

  start() {
    this.timeAtStart = performance.now();
  }
}
