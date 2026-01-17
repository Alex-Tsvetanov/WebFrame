export type Task = (done: () => void) => void;

export class SerialTaskQueue {
  private ready: boolean = true;
  private pendingTasks: Task[] = [];

  enqueueTask(operation: Task) {
    this.pendingTasks.push(operation);
    this.flushTasks();
  }

  private flushTasks() {
    if (this.ready && this.pendingTasks.length) {
      const operation = this.pendingTasks.shift()!;
      this.ready = false;
      operation(() => {
        this.ready = true;
        this.flushTasks();
      });
    }
  }
}
