type PendingTask = (done: () => void) => void;

export class SerialTaskQueue {
  private ready: boolean = true;
  private pendingTasks: PendingTask[] = [];

  enqueueTask<T>(operation: () => T): Promise<T> {
    return new Promise((resolve, reject) => {
      this.doEnqueueTask((done) => {
        try {
          const result = operation();
          resolve(result);
        } catch (err) {
          reject(err);
        } finally {
          done();
        }
      });
    });
  }

  enqueueAsyncTask<T>(operation: () => Promise<T>): Promise<T> {
    return new Promise((resolve, reject) => {
      this.doEnqueueTask((done) => {
        operation()
          .then((result) => {
            resolve(result);
          })
          .catch((err) => {
            reject(err);
          })
          .finally(done);
      });
    });
  }

  private doEnqueueTask(task: PendingTask) {
    this.pendingTasks.push(task);
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
