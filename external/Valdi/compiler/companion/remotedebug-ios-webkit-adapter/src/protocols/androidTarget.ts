//
// Copyright (C) Microsoft. All rights reserved.
//

import * as WebSocket from 'ws';
import { EventEmitter } from 'events';
import { Logger, debug } from '../logger';
import { ITarget } from '../adapters/adapterInterfaces';
import * as net from 'net';
import { raw } from 'express';
import { AdapterTarget } from './target';

export class AndroidTarget extends EventEmitter implements AdapterTarget {
    private _data: ITarget;
    private _url: string = "";
    private _target?: net.Socket;
    private _wsTools?: WebSocket;
    private _isConnected: boolean = false;
    private _messageBuffer: string[];
    private _messageFilters: Map<string, ((msg: any) => Promise<any>)[]>;
    private _toolRequestMap: Map<number, string>;
    private _adapterRequestMap: Map<number, { resolve: (arg0: any) => void; reject: (arg0: any) => void }>;
    private _requestId: number;
    private _id: string;
    private _targetBased: boolean;
    private _targetId: string;

    private _buffer = new Uint8Array();

    constructor(targetId: string, data?: ITarget) {
        super();
        this._data = data || <ITarget>{};
        this._messageBuffer = [];
        this._messageFilters = new Map<string, ((msg: any) => Promise<any>)[]>();
        this._toolRequestMap = new Map<number, string>();
        this._adapterRequestMap = new Map<number, { resolve: (arg0: any) => void; reject: (arg0: any) => void }>();
        this._requestId = 0;
        this._targetBased = false;
        this._targetId = "";

        // Chrome currently uses id, iOS usies appId
        this._id = targetId;
    }

    public get data(): ITarget {
        return this._data;
    }

    public set targetBased(isTargetBased: boolean) {
        this._targetBased = isTargetBased;
    }

    public set targetId(targetId: string) {
        this._targetId = targetId;
    }

    public kill() {
        this._target?.end();
    }

    public connectTo(url: string, wsFrom: WebSocket): void {
        if (this._target) {
            Logger.error(`Already connected`);
            return;
        }
        this._wsTools = wsFrom;
        this.directConnectTo(url);
    }

    public directConnectTo(url: string): void {
        if (this._target) {
            Logger.error(`Already connected`);
            return;
        }

        this._url = url;

        const parsedURL = new URL(`tcp://${this._data.url}`);
        this._target = net.createConnection(Number(parsedURL.port), parsedURL.hostname);
        this._target.on('error', err => {
            Logger.error(err.message);
            this.emit('socketClosed', this._id);
        });

        this._target.on('data', data => {
            this.receivedDataFromTarget(data);
        });

        this._target.on('connect', () => {
            debug(`Connection established to ${url}`);
            this._isConnected = true;
            for (let i = 0; i < this._messageBuffer.length; i++) {
                this.onMessageFromTools(this._messageBuffer[i]);
            }
            this._messageBuffer = [];
        });
        this._target.on('end', () => {
            debug('Socket is closed');
            this.emit('socketClosed', this._id);
        });
    }

    public forward(message: string): void {
        if (!this._target) {
            Logger.error('No websocket endpoint found');
            return;
        }

        this.onMessageFromTools(message);
    }

    public updateClient(wsFrom: WebSocket): void {
        if (this._target) {
            this._target.destroy();
        }
        this._target = undefined;
        this.connectTo(this._url, wsFrom);
    }

    public addMessageFilter(method: string, filter: (msg: any) => Promise<any>): void {
        if (!this._messageFilters.has(method)) {
            this._messageFilters.set(method, []);
        }

        this._messageFilters.get(method)?.push(filter);
    }

    public callTarget(method: string, params: any): Promise<any> {
        return new Promise((resolve, reject) => {
            const request = {
                id: --this._requestId,
                method: method,
                params: params,
            };

            this._adapterRequestMap.set(request.id, { resolve: resolve, reject: reject });
            this.sendToTarget(JSON.stringify(request));
        });
    }

    public fireEventToTools(method: string, params: any): void {
        const response = {
            method: method,
            params: params,
        };

        this.sendToTools(JSON.stringify(response));
    }

    public fireResultToTools(id: number, params: any): void {
        const response = {
            id: id,
            result: params,
        };

        this.sendToTools(JSON.stringify(response));
    }

    public replyWithEmpty(msg: any): Promise<any> {
        this.fireResultToTools(msg.id, {});
        return Promise.resolve(null);
    }

    private onMessageFromTools(rawMessage: string): void {
        if (!this._isConnected) {
            debug('Connection not yet open, buffering message.');
            this._messageBuffer.push(rawMessage);
            return;
        }

        // console.log('Received Tools message:', rawMessage);

        const msg = JSON.parse(rawMessage);
        const eventName = `tools::${msg.method}`;

        this._toolRequestMap.set(msg.id, msg.method);
        this.emit(eventName, msg.params);

        if (this._messageFilters.has(eventName)) {
            let sequence = Promise.resolve(msg);

            this._messageFilters.get(eventName)?.forEach(filter => {
                sequence = sequence.then(filteredMessage => {
                    return filter(filteredMessage);
                });
            });

            sequence.then(filteredMessage => {
                // Only send on the message if it wasn't completely filtered out
                if (filteredMessage) {
                    rawMessage = JSON.stringify(filteredMessage);
                    this.sendToTarget(rawMessage);
                }
            });
        } else {
            // Pass it on to the target
            this.sendToTarget(rawMessage);
        }
    }

    private receivedDataFromTarget(data: Uint8Array) {
        if (!this._buffer.length) {
            this._buffer = data;
        } else {
            const newBuffer = new Uint8Array(data.length + this._buffer.length);
            newBuffer.set(this._buffer, 0);
            newBuffer.set(data, this._buffer.length);
            this._buffer = newBuffer;
        }

        this.processBuffer();
    }

    private processBuffer() {
        for (;;) {
            if (this._buffer.length < 4) {
                return;
            }

            const view = new DataView(this._buffer.buffer);
            const size = view.getUint32(0, false);

            if (size > this._buffer.length - 4) {
                // Not enough data yet
                return;
            }

            const jsonData = new Uint8Array(this._buffer.buffer, 4, size);

            const jsonString = new TextDecoder('utf-8').decode(jsonData);

            const newBuffer = new Uint8Array(this._buffer.length - jsonData.length - 4);
            newBuffer.set(this._buffer.subarray(jsonData.length + 4), 0);
            this._buffer = newBuffer;

            this.onMessageFromTarget(jsonString);
        }
    }

    private onMessageFromTarget(rawMessage: string): void {
        // console.error('Received JS message:', rawMessage);

        let msg = JSON.parse(rawMessage);

        if (this._targetBased) {
            if (!msg.method || !msg.method.match(/^Target/)) {
                return;
            }
            if (msg.method === 'Target.dispatchMessageFromTarget') {
                rawMessage = msg.params.message;
                msg = JSON.parse(rawMessage);
            }
        }

        if ('id' in msg) {
            if (this._toolRequestMap.has(msg.id)) {
                // Reply to tool request
                let eventName = `target::${this._toolRequestMap.get(msg.id)}`;
                this.emit(eventName, msg.params);

                this._toolRequestMap.delete(msg.id);

                if ('error' in msg && this._messageFilters.has('target::error')) {
                    eventName = 'target::error';
                }

                if (this._messageFilters.has(eventName)) {
                    let sequence = Promise.resolve(msg);

                    this._messageFilters.get(eventName)?.forEach(filter => {
                        sequence = sequence.then(filteredMessage => {
                            return filter(filteredMessage);
                        });
                    });

                    sequence.then(filteredMessage => {
                        rawMessage = JSON.stringify(filteredMessage);
                        this.sendToTools(rawMessage);
                    });
                } else {
                    // Pass it on to the tools
                    this.sendToTools(rawMessage);
                }
            } else if (this._adapterRequestMap.has(msg.id)) {
                // Reply to adapter request
                const resultPromise = this._adapterRequestMap.get(msg.id);
                this._adapterRequestMap.delete(msg.id);

                if ('result' in msg) {
                    resultPromise?.resolve(msg.result);
                } else if ('error' in msg) {
                    resultPromise?.reject(msg.error);
                } else {
                    Logger.error(`Unhandled type of request message from target ${rawMessage}`);
                }
            } else {
                Logger.error(`Unhandled message from target ${rawMessage}`);
            }
        } else {
            const eventName = `target::${msg.method}`;
            this.emit(eventName, msg);

            if (this._messageFilters.has(eventName)) {
                let sequence = Promise.resolve(msg);

                this._messageFilters.get(eventName)?.forEach(filter => {
                    sequence = sequence.then(filteredMessage => {
                        return filter(filteredMessage);
                    });
                });

                sequence.then(filteredMessage => {
                    rawMessage = JSON.stringify(filteredMessage);
                    this.sendToTools(rawMessage);
                });
            } else {
                // Pass it on to the tools
                this.sendToTools(rawMessage);
            }
        }
    }

    private sendToTools(rawMessage: string): void {
        debug(`sendToTools.${rawMessage}`);
        // Make sure the tools socket can receive messages
        // console.log('Sending message to tool:', rawMessage);
        if (this._wsTools && this.isSocketConnected(this._wsTools)) {
            this._wsTools.send(rawMessage);
        }
    }

    private sendToTarget(rawMessage: string): void {
        debug(`sendToTarget.${rawMessage}`);
        if (this._targetBased) {
            const message = JSON.parse(rawMessage);
            if (!message.method.match(/^Target/)) {
                const newMessage = {
                    id: message.id,
                    method: 'Target.sendMessageToTarget',
                    params: {
                        id: message.id,
                        message: JSON.stringify(message),
                        targetId: this._targetId,
                    },
                };
                rawMessage = JSON.stringify(newMessage);
                debug(`sendToTarget.targeted.${rawMessage}`);
            }
        }

        const data = new TextEncoder().encode(rawMessage);

        const packet = new ArrayBuffer(data.length + 4);
        const view = new DataView(packet);
        view.setUint32(0, data.length, false);

        let i = 4;
        for (const b of data) {
            view.setUint8(i, b);
            i++;
        }

        // Make sure the target socket can receive messages
        if (this._target) {
            this._target.write(new Uint8Array(packet));
        } else {
            // this._target.send(rawMessage);
            // The socket has closed, we should send this message up to the parent
            this._target = undefined;
            this.emit('socketClosed', this._id);
        }
    }

    private isSocketConnected(ws: WebSocket): boolean {
        return ws && ws.readyState === WebSocket.OPEN;
    }
}
