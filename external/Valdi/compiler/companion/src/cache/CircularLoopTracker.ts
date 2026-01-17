export type ICircularLoop = string[];

export const enum CircularLoopTrackerPushResult {
  /**
   * First time the entry was visited. The entry is not circular
   */
  nonCircular,

  /**
   * The entry was already visited and is in the stack. A circular loop
   * was detected.
   */
  circular,

  /**
   * The entry was already visited but is not in the stack.
   */
  alreadyVisited,
}

export class CircularLoopTracker {
  get stackSize(): number {
    return this.stack.length;
  }

  private stack: string[] = [];
  private allVisited = new Set<string>();
  private loopsByEntry: Map<string, Set<string>> | undefined = undefined;

  private handleAlreadyVisited(entry: string): CircularLoopTrackerPushResult {
    const existingLoops = this.loopsByEntry?.get(entry);
    let pendingConnections: string[] | undefined;

    if (existingLoops) {
      for (const innerEntry of existingLoops) {
        if (this.stack.indexOf(innerEntry) >= 0) {
          if (!pendingConnections) {
            pendingConnections = [];
          }
          pendingConnections.push(innerEntry);
        }
      }
    }

    if (!pendingConnections) {
      return CircularLoopTrackerPushResult.alreadyVisited;
    }

    // We are attempting pushing an item that is already visited and is part of a circle
    // that is on the stack. We need to treat the push as a circular loop

    for (const entryToProcess of pendingConnections) {
      const index = this.stack.indexOf(entryToProcess);
      this.handleCircularLoop(index);
    }

    return CircularLoopTrackerPushResult.circular;
  }

  push(entry: string): CircularLoopTrackerPushResult {
    if (this.allVisited.has(entry)) {
      const indexOfEntry = this.stack.indexOf(entry);
      if (indexOfEntry >= 0) {
        this.handleCircularLoop(indexOfEntry);
        return CircularLoopTrackerPushResult.circular;
      } else {
        return this.handleAlreadyVisited(entry);
      }
    }

    this.allVisited.add(entry);
    this.stack.push(entry);

    return CircularLoopTrackerPushResult.nonCircular;
  }

  pop(): void {
    this.stack.pop();
  }

  getCircularEntriesForEntry(entry: string): Set<string> | undefined {
    return this.loopsByEntry?.get(entry);
  }

  getResolvedCircularLoops(): ICircularLoop[] | undefined {
    if (!this.loopsByEntry) {
      return undefined;
    }

    const allLoops: Set<string>[] = [];

    for (const loop of this.loopsByEntry.values()) {
      if (allLoops.indexOf(loop) < 0) {
        allLoops.push(loop);
      }
    }

    return allLoops.map((loop) => [...loop.values()].sort()).sort((l, r) => l.length - r.length);
  }

  private addEntriesToLoop(left: string, right: string, loopsByEntry: Map<string, Set<string>>) {
    let existingLeftLoop = loopsByEntry.get(left);
    let existingRightLoop = loopsByEntry.get(right);

    if (existingLeftLoop === existingRightLoop) {
      if (!existingLeftLoop) {
        // First time we see both entries
        const loop = new Set<string>([left, right]);
        loopsByEntry.set(left, loop);
        loopsByEntry.set(right, loop);
      } else {
        // Both entries have already been seen and been connected
        return;
      }
    }

    if (existingLeftLoop) {
      if (existingRightLoop) {
        // Each one of the entries is already in a loop. we will take the entries from right
        // and move them into the loop from left
        const entriesInRight = [...existingRightLoop.values()];

        for (const rightEntry of entriesInRight) {
          loopsByEntry.delete(rightEntry);
        }
        for (const rightEntry of entriesInRight) {
          this.addEntriesToLoop(left, rightEntry, loopsByEntry);
        }
      } else {
        // Add 'right' to the loop
        existingLeftLoop.add(right);
        loopsByEntry.set(right, existingLeftLoop);
      }
    } else if (existingRightLoop) {
      // Add 'left' to the loop
      existingRightLoop.add(left);
      loopsByEntry.set(left, existingRightLoop);
    }
  }

  private handleCircularLoop(startStackIndex: number): void {
    let loopsByEntry = this.loopsByEntry;
    if (!loopsByEntry) {
      loopsByEntry = new Map();
      this.loopsByEntry = loopsByEntry;
    }

    for (let i = startStackIndex; i < this.stack.length - 1; i++) {
      const left = this.stack[i];
      const right = this.stack[i + 1];

      this.addEntriesToLoop(left, right, loopsByEntry);
    }
  }
}
