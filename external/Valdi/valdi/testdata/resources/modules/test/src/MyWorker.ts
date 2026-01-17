console.log('inside worker');
onmessage = () => {
  console.log('worker.onmessage');
  postMessage('works');
};
