//
// Copyright (C) Microsoft. All rights reserved.
//

import * as WebSocket from 'ws';
import { AdapterTarget } from '../protocols/target';
import { ITarget } from './adapterInterfaces';
import { EventEmitter } from 'ws';
import { Adapter } from './adapter';

export class UniversalAdapter extends EventEmitter {
    constructor(private adapters: Adapter<any>[]) {
        super();
        for (const adapter of adapters) {
            adapter.on('socketClosed', id => {
                this.emit('socketClosed', id);
            });
        }
    }

    public async getTargets(): Promise<ITarget[]> {
        const allTargets: ITarget[] = [];
        for (const adapter of this.adapters) {
            allTargets.push(...await adapter.getTargets());
        }

        return allTargets;
    }

    public connectTo(url: string, wsFrom: WebSocket): AdapterTarget | undefined {
        for (const adapter of this.adapters) {
            const target = adapter.connectTo(url, wsFrom);
            if (target) {
                return target;
            }
        }

        throw new Error(`Target not found for ${url}`);
    }

    public closeConnectionTo(url: string): void {
        for (const adapter of this.adapters) {
            adapter.closeConnectionTo(url);
        }
    }

    public async start() {
        for (const adapter of this.adapters) {
            await adapter.start();
        }
    }

    public stop() {
        for (const adapter of this.adapters) {
            adapter.stop();
        }
    }

    public forwardTo(targetId: string, message: string): void {
        for (const adapter of this.adapters) {
            adapter.forwardTo(targetId, message);
        }
    }

    public forceRefresh() {
        for (const adapter of this.adapters) {
            adapter.forceRefresh();
        }
    }
}
