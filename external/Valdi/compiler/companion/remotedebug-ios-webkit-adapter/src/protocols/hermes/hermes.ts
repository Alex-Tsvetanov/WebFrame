//
// Copyright (C) Microsoft. All rights reserved.
//

import { Logger } from '../../logger';
import { ProtocolAdapter } from '../protocol';
import { AdapterTarget } from '../target';
import { IAndroidDeviceTarget } from '../../adapters/adapterInterfaces';

export class HermesProtocol extends ProtocolAdapter {
    protected shouldDeferScriptParsed: boolean = true;
    protected deferredScriptParsedCalls: (() => void)[] = [];
    protected runtimeEnabled: boolean = false;

    constructor(target: AdapterTarget, version: String) {
        super(target);

        const id = (target.data.metadata as IAndroidDeviceTarget).deviceId;
        target
            .callTarget('Valdi.connectDebuggerTo', { id: id })
            .then((result) => {})
            .catch((err) => {
                console.error('[HermesProtocol] Unable to connect debugger to id ' + id, err.message);
                target.kill();
            });

        this._target.addMessageFilter('tools::Runtime.enable', (msg) => this.onRuntimeEnable(msg));
        this._target.addMessageFilter('target::Runtime.enable', (msg) => this.onDidRuntimeEnable(msg));

        this._target.addMessageFilter('target::Runtime.runIfWaitingForDebugger', (msg) =>
            this.onTargetRunIfWaitingForDebugger(msg),
        );
        this._target.addMessageFilter('tools::Runtime.callFunctionOn', (msg) => this.callFunctionOn(msg));
        this._target.addMessageFilter('target::Debugger.scriptParsed', (msg) => this.onScriptParsed(msg));
    }
    private callFunctionOn(msg: any): Promise<any> {
        const result = {
            result: {
                type: 'undefined',
            },
        };
        this._target.fireResultToTools(msg.id, result);
        return Promise.resolve();
    }

    private onRuntimeEnable(msg: any): Promise<any> {
        return Promise.resolve(msg);
    }

    private onDidRuntimeEnable(msg: any): Promise<any> {
        this.runtimeEnabled = true;
        return Promise.resolve(msg);
    }

    private onTargetRunIfWaitingForDebugger(msg: any): Promise<any> {
        if (this.runtimeEnabled) {
            this.shouldDeferScriptParsed = false;
            this.deferredScriptParsedCalls.forEach((callback) => {
                callback();
            });
            this.deferredScriptParsedCalls = [];
        }
        return Promise.resolve(msg);
    }

    private onScriptParsed(msg: any): Promise<any> {
        return new Promise((resolve) => {
            if (this.shouldDeferScriptParsed) {
                // We're sending the response after a delay, otherwise this seems to be sent
                // too early for VS Code to pick up the parsed sources
                this.deferredScriptParsedCalls.push(() => {
                    resolve(msg);
                });
                return;
            }
            resolve(msg);
        });
    }
}
