import { Component } from 'valdi_core/src/Component';
import Worker from 'worker/src/Worker';

export class WorkerTest extends Component {
  worker: Worker | null = null;

  callWorker(callback: (res: string) => void) {
    this.worker = new Worker('test/src/MyWorker');
    this.worker.onmessage = e => {
      callback(e.data as string);
    };
    this.worker.postMessage('hi');
  }

  onRender() {
    <view />
  }
}
