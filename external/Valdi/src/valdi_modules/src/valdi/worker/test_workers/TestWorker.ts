import { inWorker } from 'worker/src/Worker';

console.log('worker: started!');

onmessage = e => {
  const data = e.data as [string, number];
  if (data[0] === 'hello' && data[1] === 123 && inWorker()) {
    console.log('worker: received host message');
    console.log('worker: sending reply message');
    postMessage(['world', 456]);
  } else if (data[0] === 'close') {
    close();
    postMessage(['afterclose', 456]);
  } else {
    postMessage('fail');
  }
  close();
};
