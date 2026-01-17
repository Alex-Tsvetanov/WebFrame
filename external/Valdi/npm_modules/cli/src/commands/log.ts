import fs from 'fs';
import path from 'path';
import { CliError } from '../core/errors';
import { getUserChoice } from '../utils/cliUtils';
import { makeCommandHandler } from '../utils/errorUtils';
import * as fileUtils from '../utils/fileUtils';

function readAndWatchFile(filePath: string) {
  filePath = fileUtils.resolveFilePath(filePath);

  let fileSize = 0;
  let isWatching = false;
  const abortController = new AbortController();

  const readInitialContent = () => {
    const fileStream = fs.createReadStream(filePath, { encoding: 'utf8', start: 0 });

    fileStream.on('data', chunk => {
      process.stdout.write(chunk);
    });

    fileStream.on('end', () => {
      fileSize = fs.statSync(filePath).size;

      if (!isWatching) {
        startWatching();
      }
    });
  };

  const startWatching = () => {
    isWatching = true;
    fs.watch(filePath, { signal: abortController.signal }, eventType => {
      if (eventType === 'change') {
        const newFileSize = fs.statSync(filePath).size;
        if (newFileSize > fileSize) {
          const readStream = fs.createReadStream(filePath, { encoding: 'utf8', start: fileSize });
          readStream.on('data', chunk => {
            process.stdout.write(chunk);
          });
          fileSize = newFileSize;
        }
      } else if (eventType === 'rename') {
        isWatching = false;
        if (fs.existsSync(filePath)) {
          // When files is written, sometimes the first reported event
          // occurs when the write is incomplete, so verifying the file exists
          // before reading and restarting the watch
          readInitialContent();
        }
      }
    });
  };

  readInitialContent();

  // Liten for (Ctrl+D) to exit as otherwise it would not do anything
  process.stdin.on('end', () => {
    abortController.abort();
  });

  // Keep the process running
  process.stdin.resume();
}

async function showValdiLogs() {
  const userConfig = fileUtils.getUserConfig();

  if (userConfig.logs_output_dir === undefined) {
    throw new CliError("Missing 'logs_output_dir' config value in config.yaml");
  }

  const allFilePaths = fileUtils.getFilesSortedByUpdatedTime(userConfig.logs_output_dir).filter(path => {
    return path.endsWith('.log');
  });

  if (allFilePaths.length <= 0) {
    throw new CliError(
      'No logs available. Please start up hot reloader and the app first to make sure there are logs available',
    );
  }

  let logPath = '';

  if (allFilePaths.length === 1) {
    logPath = allFilePaths[0] as string;
  } else {
    const choices = allFilePaths.map((filePath, index) => ({
      name: `${index + 1}. ${path.basename(filePath)}`,
      value: filePath,
    }));

    logPath = await getUserChoice(
      choices,
      'Please select the log file to use (this list is ordered by modification time, the most updated log will be displayed first):',
    );

    console.log(logPath);
  }

  // Read and show log file content
  readAndWatchFile(logPath);
}

export const command = 'log';
export const describe = 'Streams Valdi logs from the connected device to console';
export const builder = () => {};
export const handler = makeCommandHandler(showValdiLogs);
