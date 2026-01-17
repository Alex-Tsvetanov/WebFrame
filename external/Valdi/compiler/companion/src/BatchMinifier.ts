import * as fs from 'fs';

if (typeof btoa !== 'undefined') {
  // Replace btoa function before loading terser as it is broken on node js v16
  global.btoa = function btoa(str: string): string {
    return Buffer.from(str).toString('base64');
  };
}

import * as terser from 'terser';
import * as _url from 'url';

export interface BatchMinifyResult {
  fileURL: string;
  uglified?: string;
  error?: string;
}

export type CommonMinifyOptions = terser.MinifyOptions;

interface MinifyOutput {
  code?: string;
  error?: Error;
}

const defaultOptions: CommonMinifyOptions = {
  ecma: 6,
  safari10: true,
  mangle: {
    toplevel: true,
    keep_classnames: true,
  },
};

export const batchMinify = (minifierName: 'terser', fileURLs: string[], providedOptions?: CommonMinifyOptions) => {
  let minifier: Minifier<CommonMinifyOptions>;
  switch (minifierName) {
    case 'terser':
      minifier = terser;
      break;
    default:
      throw new Error('Unknown minifier');
  }

  const options = providedOptions ?? defaultOptions;
  return doBatchMinify(minifier, fileURLs, options);
};

interface Minifier<MinifyOptionsT> {
  minify(
    files:
      | string
      | string[]
      | {
          [file: string]: string;
        },
    options?: MinifyOptionsT | undefined,
  ): MinifyOutput;
}

function deepCopy(obj: any): any {
  if (typeof obj !== 'object') {
    return obj;
  }

  if (Array.isArray(obj)) {
    return obj.map(deepCopy);
  } else {
    const out: any = {};

    for (const key in obj) {
      out[key] = deepCopy(obj[key]);
    }

    return out;
  }
}

const doBatchMinify = <MinifyOptionsT>(
  minifier: Minifier<MinifyOptionsT>,
  fileURLs: string[],
  options: MinifyOptionsT | undefined,
): Promise<BatchMinifyResult[]> => {
  const promises = fileURLs.map((fileURL) => {
    const promise = new Promise((resolve: (value: BatchMinifyResult) => void, reject) => {
      const path = _url.fileURLToPath(fileURL);
      fs.readFile(path, (err, buffer) => {
        if (err) {
          resolve({ fileURL, error: err.toString() });
          return;
        }

        const original = buffer.toString();

        // terser sometimes modifies the input options, this makes sure
        // we give a unique options instance each time.
        const minifyOptions = deepCopy(options);
        const result = minifier.minify(original, minifyOptions);

        if (result.error) {
          resolve({ fileURL, error: result.error.toString() });
          return;
        }

        const uglified = result.code;

        resolve({ fileURL, uglified });
      });
    });
    return promise;
  });
  return Promise.all(promises);
};
