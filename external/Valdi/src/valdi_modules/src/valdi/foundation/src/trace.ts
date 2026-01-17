/**
 * Wraps the Valdi trace function so that we can optionally require it. This is useful so we can run
 * unit tests without requiring Valdi core.
 */

declare const require: any;

let traceFunction: any;
try {
  traceFunction = require('valdi_core/src/utils/Trace').trace;
} catch (err: any) {
  console.log(`Could not load Trace module: ${err.message}`);
}

export function trace<T>(tag: string, cb: () => T): T {
  if (traceFunction) {
    return traceFunction(tag, cb);
  } else {
    return cb();
  }
}
