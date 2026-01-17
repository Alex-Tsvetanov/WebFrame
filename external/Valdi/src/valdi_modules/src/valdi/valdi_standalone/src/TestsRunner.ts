import { fs } from 'file_system/src/FileSystem';
import { getModuleJsPaths } from 'valdi_core/src/Valdi';
import { getStandaloneRuntime } from 'valdi_standalone/src/ValdiStandalone';
import 'jasmine/src/jasmine';
import { ArgumentsParser } from './ArgumentsParser';
import { prepareTests } from './JasmineBootstrap';
import { mockNativeModules } from './NativeModules';

declare function require(path: string, noProxy: boolean): any;

interface Configuration {
  showColors: boolean;
  filesToImport: string[];
  modulesToImport: string[];
  allowIncompleteTestRun: boolean;
  testErrorAllowList: string[];
  iterationsCount?: number;
  codeCoverage?: boolean;
  codeCoverageResultFile?: string;
  instrumentedFilesResult?: string;
  junitReportOutputDir?: string;
  junitReportFilename?: string;
  strictErrorHandling?: boolean;
  retryFailures?: boolean;
  jasmineSeed?: number;
}

function parseArguments(): Configuration {
  const parser = new ArgumentsParser('ValdiTests', getStandaloneRuntime().arguments);

  const filesToImport = parser.addArray('--include', 'Path of a file to evaluate', false);
  const modulesToImport = parser.addArray('--include_module', 'Path of a module to evaluate', false);
  const codeCoverage = parser.addFlag('--code_coverage', 'Enable Code Coverage reports', false);
  const codeCoverageResultFile = parser.add('--code_coverage_result', 'Path to Coverage file result', false);
  const disableColors = parser.addFlag('--no_color', 'Whether to disable color terminal output', false);
  const iterationsCount = parser.addNumber('--iterations', 'Specify number of iterations', false);
  const strictErrorHandling = parser.addFlag(
    '--strict_error_handling',
    'Whether uncaught errors should result in the tests failing',
    false,
  );
  const testErrorAllowListFile = parser.addString(
    '--test_error_allow_list_file',
    'A file with a list of paths to tests that are allowed to fail',
    false,
  );
  const allowIncompleteTestRun = parser.addFlag(
    '--allow_incomplete_test_run',
    `fit() and fdescribe() can be used to only run specific tests.
This is only useful in development, so when the test runner is not running in hot reload mode, it will fail if it detects that the test run is _incomplete_.
Use --allow_incomplete_test_run to allow the test run to succeed even if it's incomplete`,
    false,
  );
  const junitReportOutputDir = parser.addString(
    '--junit_report_output_dir',
    'Path to directory where to output junit test report files',
    false,
  );
  const junitReportFilename = parser.addString(
    '--junit_report_filename',
    'Filename of the output junit test report file',
    false,
  );
  const jasmineSeed = parser.addNumber(
    '--jasmine_seed',
    'Seed to use for randomizing jasmine test execution order',
    false,
  );

  const retryFailures = parser.addFlag('--retry_failures', 'Run tests in retry mode', false);

  parser.parse();

  if (!filesToImport.isSet && !modulesToImport.isSet) {
    parser.printDescription();
    console.error('--include or --include_module need to be provided');
  }

  let testErrorAllowList: string[] = [];
  if (testErrorAllowListFile.value) {
    const fileContent = fs.readFileSync(testErrorAllowListFile.value) as ArrayBuffer;
    const decoder = new TextDecoder('utf-8');
    const textContent = decoder.decode(fileContent);
    testErrorAllowList = textContent
      .split('\n')
      .map(line => line.trim())
      .filter(line => line.length > 0);
  }

  return {
    showColors: !(disableColors.value ?? false),
    filesToImport: filesToImport.value ?? [],
    modulesToImport: modulesToImport.value ?? [],
    allowIncompleteTestRun: allowIncompleteTestRun.value ?? false,
    testErrorAllowList: testErrorAllowList,
    iterationsCount: iterationsCount.value,
    codeCoverage: codeCoverage.value,
    codeCoverageResultFile: codeCoverageResultFile.value,
    junitReportOutputDir: junitReportOutputDir.value,
    junitReportFilename: junitReportFilename.value,
    strictErrorHandling: strictErrorHandling.value,
    retryFailures: retryFailures.value,
    jasmineSeed: jasmineSeed.value,
  };
}

mockNativeModules();

const configuration = parseArguments();

function isTestFile(path: string): boolean {
  return path.startsWith('test/') || path.endsWith('.spec');
}

prepareTests(configuration, (beforeLoadCallback: (fileToImport: string) => void) => {
  const resolvedFilesToImport = [...configuration.filesToImport];
  for (const moduleToImport of configuration.modulesToImport) {
    const filesInModule = getModuleJsPaths(moduleToImport);

    for (const fileInModule of filesInModule) {
      if (isTestFile(fileInModule)) {
        resolvedFilesToImport.push(`${moduleToImport}/${fileInModule}`);
      }
    }
  }

  for (const fileToImport of resolvedFilesToImport) {
    if (fileToImport === module.path) {
      // Don't self-load the TestsRunner itself
      continue;
    }
    beforeLoadCallback(fileToImport);
    require(fileToImport, true);
  }
});
