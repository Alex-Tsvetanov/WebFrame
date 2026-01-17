import { paramsFromUrl } from 'coreutils/src/url';

const echoParamValue = paramsFromUrl(location.search)['echo'];

onmessage = e => {
  if (e.data === 'ping' && echoParamValue) {
    postMessage(echoParamValue);
  }
  close();
};
