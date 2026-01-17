import { Stopwatch } from 'valdi_core/src/utils/Stopwatch';

const TARGET_BENCHMARK_DURATION_MS = 2000.0;

export function randomInt(min: number, max: number): number {
  return Math.round((max - min) * Math.random()) + min;
}

export function randomString(length: number): string {
  const charCodes: number[] = [];
  for (let index = 0; index < length; index++) {
    const code = randomInt(65, 90);

    charCodes.push(code);
  }

  return String.fromCharCode(...charCodes);
}

export interface IBenchmarkContext {
  pauseTiming(): void;
  resumeTiming(): void;

  benchDurationMs: number;

  bench(name: string, func: () => void): void;
}

interface BenchmarkResult {
  name: string;
  ops: number;
}

class BenchmarkContext implements IBenchmarkContext {
  private sw = new Stopwatch();
  private results: BenchmarkResult[] = [];

  benchDurationMs = TARGET_BENCHMARK_DURATION_MS;

  pauseTiming() {
    this.sw.pause();
  }

  resumeTiming() {
    this.sw.resume();
  }

  bench(name: string, func: () => void) {
    console.log('Testing', name, '...');
    const sw = this.sw;
    sw.start();

    let iterations = 0;
    do {
      iterations++;
      func();
    } while (sw.elapsedMs() < this.benchDurationMs);

    const elapsed = sw.elapsed();
    const ops = iterations / elapsed.seconds;
    this.results.push({ name, ops });
  }

  complete() {
    if (!this.results.length) {
      console.error('No benchmark functions registered.');
      return;
    }

    const sortedResults = this.results.sort((left, right) => right.ops - left.ops);

    console.log('Printing results:');

    const fastest = sortedResults[0];

    for (const result of sortedResults) {
      let suffix: string;
      if (result === fastest) {
        suffix = 'fastest';
      } else {
        const diffPct = ((1.0 - result.ops / fastest.ops) * 100).toPrecision(2);
        suffix = `${diffPct}% slower`;
      }

      console.log(`${result.name} -- ${result.ops} op/s (${suffix})`);
    }
  }
}

export function performBenchmark(func: (context: IBenchmarkContext) => void) {
  const benchmarkName = func.name;
  console.log();
  console.log('Starting benchmark', benchmarkName, '...');
  const ctx = new BenchmarkContext();
  func(ctx);
  ctx.complete();
}
