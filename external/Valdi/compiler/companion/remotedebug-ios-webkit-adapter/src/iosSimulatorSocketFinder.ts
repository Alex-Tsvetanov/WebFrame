import { spawn, ChildProcess, exec } from 'child_process';

export class IOSSimulatorSocketFinder {
    private timer?: NodeJS.Timeout;
    private knownSockets: string[] | undefined;
    private waitingQueries: ((sockets: string[]) => void)[] = [];
    private onSocketsChanged?: (sockets: string[]) => void;

    public observeSockets(callback: (sockets: string[]) => void) {
        this.onSocketsChanged = callback;
    }

    public start() {
        this.scheduleTick();
    }

    public stop() {
        clearTimeout(this.timer);
        this.timer = undefined;
    }

    public async listKnownSockets(): Promise<string[]> {
        if (this.knownSockets) {
            return new Promise(resolve => resolve(this.knownSockets || []));
        } else {
            return new Promise(resolve => {
                this.waitingQueries.push(resolve);
            });
        }
    }

    private arraysEqual<T>(a: Array<T>, b: Array<T>) {
        if (a === b) return true;
        if (a == null || b == null) return false;
        if (a.length !== b.length) return false;

        for (let i = 0; i < a.length; ++i) {
            if (a[i] !== b[i]) return false;
        }
        return true;
    }

    private updateKnownSockets(knownSockets: string[]) {
        if (this.knownSockets) {
            if (this.arraysEqual(this.knownSockets, knownSockets)) {
                return;
            }
        }

        this.knownSockets = knownSockets;
        this.waitingQueries.forEach(resolve => {
            resolve(knownSockets);
        });
        this.waitingQueries = [];

        this.onSocketsChanged?.(this.knownSockets);
    }

    private scheduleTick() {
        this.timer = setTimeout(() => {
            this.tick();
        }, 1000);
    }

    private tick() {
        exec('lsof -U -F | grep com.apple.webinspectord_sim.socket | uniq', (error, stdout, stderr) => {
            if (error) {
                console.log(`error: ${error.message}`);
                return;
            }
            if (stderr) {
                console.error(`stderr: ${stderr}`);
                return;
            }
            const sockets = stdout
                .split('\n')
                .filter(line => line.length > 1)
                .map(line => line.substr(1));

            const uniquedSockets = Array.from(new Set(sockets)).sort();
            this.updateKnownSockets(uniquedSockets);

            if (this.timer) {
                this.scheduleTick();
            }
        });
    }
}
