export class Timer {
  startMs: number;
  constructor() {
    this.startMs = this.nowMs();
  }
  reset() {
    this.startMs = this.nowMs();
  }
  elapsedMs() {
    return this.nowMs() - this.startMs;
  }
  private nowMs() {
    return new Date().getTime();
  }
}
