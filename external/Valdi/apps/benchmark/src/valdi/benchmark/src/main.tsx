import { question, print } from './cpp';
import { runProtoImport } from './ProtoImportTest';

import { RequireFunc } from 'valdi_core/src/IModuleLoader';
declare const require: RequireFunc;
const microbench = require('benchmark/src/microbench');

declare const global: any;

const tests = [
  {
    name: 'MicroBench',
    body: () => {
      const argv = ['microbench'];
      microbench.setConsole({ log: print });
      microbench.main(argv.length, argv, global);
    },
  },
  {
    name: 'Proto Import',
    body: () => {
      const importTime = runProtoImport('benchmark/src/proto.protodecl');
      const importTimeNoIndex = runProtoImport('benchmark/src/proto_noidx.protodecl');
      print(`Proto (indexed) imported in ${importTime} ms`);
      print(`Proto (noindex) imported in ${importTimeNoIndex} ms`);
    },
  },
];

function readChoice(): string {
  print(`Choose the test to run ('q' to exit):`);
  tests.forEach((t, i) => {
    print(`${i + 1}. ${t.name}`);
  });
  return question(': ');
}

while (true) {
  const choice = readChoice();
  if (choice == 'q') break;
  const n = Math.floor(Number(choice) - 1);
  if (n >= 0 && n < tests.length) {
    tests[n].body();
  } else {
    print('Invalid choice.');
  }
}
