const isProcessRunning = function (pid: number) {
  try {
    // yes, this really is the way to check if a process is running
    return process.kill(pid, 0);
  } catch (e: any) {
    return e.code === 'EPERM';
  }
};

export class ZombieKiller {
  constructor(private shutdownCallback: () => void) {}

  private intervalHandle?: NodeJS.Timeout;

  private stopAndNotify() {
    this.stop();
    this.shutdownCallback();
  }

  stop() {
    clearInterval(this.intervalHandle);
    this.intervalHandle = undefined;
  }

  start() {
    this.intervalHandle = setInterval(() => {
      const parentPID = process.ppid;
      if (parentPID === 1) {
        console.error('Parent pid is 1, that means the parent has died. Killing self');
        this.stopAndNotify();
        return;
      }

      const isParentRunning = isProcessRunning(parentPID);
      if (!isParentRunning) {
        console.error('Parent died, killing self');
        this.stopAndNotify();
        return;
      }
    }, 1000);
  }
}
