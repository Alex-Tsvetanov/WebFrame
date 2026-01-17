import 'ts-jest';
import { CircularLoopTracker, CircularLoopTrackerPushResult } from './CircularLoopTracker';

describe('CircularLoopTracker', () => {
  it('doesnt detect circular loops on non circular visit', () => {
    const tracker = new CircularLoopTracker();
    // 1 -> 2 -> 3

    expect(tracker.push('1')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('2')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('3')).toBe(CircularLoopTrackerPushResult.nonCircular);

    tracker.pop();
    tracker.pop();
    tracker.pop();

    expect(tracker.getResolvedCircularLoops()).toBeUndefined();
  });

  it('detects already visited entries', () => {
    const tracker = new CircularLoopTracker();
    // 1 -> 2 -> 3
    // 1 -> 3

    expect(tracker.push('1')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('2')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('3')).toBe(CircularLoopTrackerPushResult.nonCircular);

    expect(tracker.stackSize).toBe(3);
    tracker.pop();
    tracker.pop();

    expect(tracker.stackSize).toBe(1);
    expect(tracker.push('3')).toBe(CircularLoopTrackerPushResult.alreadyVisited);
    tracker.pop();
    expect(tracker.stackSize).toBe(0);

    expect(tracker.getResolvedCircularLoops()).toBeUndefined();
  });

  it('detects single circular loops', () => {
    const tracker = new CircularLoopTracker();
    // 1 -> 2 -> 3 -> 1

    expect(tracker.push('1')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('2')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('3')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('1')).toBe(CircularLoopTrackerPushResult.circular);

    expect(tracker.stackSize).toBe(3);

    tracker.pop();
    tracker.pop();
    tracker.pop();

    expect(tracker.getResolvedCircularLoops()).toEqual([['1', '2', '3']]);
  });

  it('detects complex circular loops', () => {
    const tracker = new CircularLoopTracker();
    // 1 -> 2 -> 3 -> 1
    // 1 -> 2 -> 4 -> 2
    // 1 -> 5 -> 6 -> 5
    // 1 -> 9 -> 2
    // 1 -> 7 -> 8

    expect(tracker.push('1')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('2')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('3')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('1')).toBe(CircularLoopTrackerPushResult.circular);

    expect(tracker.stackSize).toBe(3);
    tracker.pop();

    expect(tracker.push('4')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('2')).toBe(CircularLoopTrackerPushResult.circular);

    expect(tracker.stackSize).toBe(3);
    tracker.pop();
    tracker.pop();

    expect(tracker.push('5')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('6')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('5')).toBe(CircularLoopTrackerPushResult.circular);

    expect(tracker.stackSize).toBe(3);
    tracker.pop();
    tracker.pop();

    expect(tracker.push('9')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('2')).toBe(CircularLoopTrackerPushResult.circular);
    tracker.pop();

    expect(tracker.push('7')).toBe(CircularLoopTrackerPushResult.nonCircular);
    expect(tracker.push('8')).toBe(CircularLoopTrackerPushResult.nonCircular);

    expect(tracker.stackSize).toBe(3);
    tracker.pop();
    tracker.pop();
    tracker.pop();

    expect(tracker.getResolvedCircularLoops()).toEqual([
      ['5', '6'],
      ['1', '2', '3', '4', '9'],
    ]);
  });
});
