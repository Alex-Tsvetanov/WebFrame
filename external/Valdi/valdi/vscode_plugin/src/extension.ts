import * as vscode from 'vscode';
import { promises as fs } from 'fs';
import * as path from 'path';
import * as os from 'os';
import { Tail } from 'tail';
import { FileChangeInfo } from 'fs/promises';


const logsDir = path.resolve(os.homedir(), '.valdi/logs/');
let watcher: ValdiLogWatcher | undefined;

export async function activate(context: vscode.ExtensionContext) {
    console.log('𝄞 valdi-vivaldi is now active 𝄞');

    watcher = new ValdiLogWatcher();
    await watcher.start();
}

export function deactivate() {
    if (watcher) {
        watcher.stop();
    }
}

// Monitor valdi logs directory and tail the most recent log file into VS Code output window.
class ValdiLogWatcher {
    private mostRecentLogFileName: string | undefined;
    private tailer: Tail | undefined;

    private watcherAbortController: AbortController;
    private watcher: AsyncIterable<FileChangeInfo<string>> | undefined;

    private outputChannel: vscode.OutputChannel;
    private everShowedOutputChannel = false;

    constructor() {
        this.outputChannel = vscode.window.createOutputChannel("Valdi Device Logs");
        this.watcherAbortController = new AbortController();
    }

    async start() {
        try {
            await fs.access(logsDir);
        } catch (error) {
            await fs.mkdir(logsDir);
        }

        await this.update();

        this.watcher = fs.watch(logsDir, { signal: this.watcherAbortController.signal });

        for await (const event of this.watcher) {
            if (event.eventType === "change" && event.filename === this.mostRecentLogFileName) {
                continue;
            }
            await this.update();
        }
    }

    stop() {
        if (this.watcher) {
            this.watcherAbortController.abort();
            this.watcher = undefined;
        }
    }

    private async update() {
        const currentMostRecentLogFileName = await this.getMostRecentLogFilePath();
        if (currentMostRecentLogFileName === this.mostRecentLogFileName) {
            return;
        }
        this.stopTailingLog();

        this.mostRecentLogFileName = currentMostRecentLogFileName;

        this.startTailingLog();
    }

    private async getMostRecentLogFilePath() {
        const fileNames = await fs.readdir(logsDir);
        const fileNamesStats = await Promise.all(
            fileNames.map(async (fileName) => {
                const stat = await fs.stat(path.join(logsDir, fileName));
                return {
                    name: fileName,
                    stat: stat
                };
            })
        );

        const sortedFiles = fileNamesStats
            .filter((fileNameStats) => fileNameStats.stat.isFile())
            .sort((a, b) => {
                return b.stat.mtime.getTime() - a.stat.mtime.getTime();
            })
            .map((fileNameStats) => {
                return fileNameStats.name;
            });

        const mostRecentLogFileName = sortedFiles[0];
        return mostRecentLogFileName;
    }

    private stopTailingLog() {
        if (!this.tailer) {
            return;
        }

        this.tailer.unwatch();
        this.tailer = undefined;
    }

    private async startTailingLog() {
        const channel = this.outputChannel;
        const fileName = this.mostRecentLogFileName!;

        console.log(`Switching to ${fileName}`);

        const logPath = path.resolve(logsDir, fileName);
        channel.clear();
        channel.appendLine(`------ Switching to ${fileName} ------`);
        const existingLogContents = await fs.readFile(logPath, { encoding: 'utf8' });
        channel.append(existingLogContents);

        this.tailer = new Tail(logPath);
        this.tailer.on("line", (data) => {
            if (!this.everShowedOutputChannel) {
                channel.show(true);
                this.everShowedOutputChannel = true;
            }

            channel.appendLine(data);
        });

        this.tailer.on("error", (error) => {
            console.error("Valdi Tail error", error);
            channel.appendLine(`[VALDI LOG ERROR] ${error}`);
        });
        this.tailer.watch();
    }
}

