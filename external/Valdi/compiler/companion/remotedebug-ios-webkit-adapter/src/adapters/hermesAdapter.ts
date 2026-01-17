//
// Copyright (C) Microsoft. All rights reserved.
//

import * as WebSocket from 'ws';
import { Logger, debug } from '../logger';
import { AndroidTarget } from '../protocols/androidTarget';
import { HermesProtocol } from '../protocols/hermes/hermes';
import { Adapter } from './adapter';
import { AdapterCollection } from './adapterCollection';
import { IAndroidDeviceTarget, ITarget } from './adapterInterfaces';

export type HermesDevice = { port: number };

function promiseWithTimeout<T>(
    promise: Promise<T>,
    ms: number,
    timeoutError = new Error('Promise timed out'),
): Promise<T> {
    const timeout = new Promise<never>((_, reject) => {
        setTimeout(() => {
            reject(timeoutError);
        }, ms);
    });
    return Promise.race<T>([promise, timeout]);
}

export class HermesAdapter extends AdapterCollection<AndroidTarget> {
    private _deviceMap: Map<number, AndroidTarget> = new Map<number, AndroidTarget>();

    constructor(id: string, socket: string) {
        super(id, socket, {}, (targetId, targetData) => new AndroidTarget(targetId, targetData));
        this._deviceMap = new Map<number, AndroidTarget>();
    }

    private targetClose(port: number) {
        Logger.error(`Closing Hermes debug device on port ${port}`);
        this._deviceMap.get(port)?.kill();
        this._deviceMap.delete(port);
        this._adapters.forEach((adapter, adapterId) => {
            if (adapterId.includes(`${this.id}_${port}`)) {
                this._adapters.delete(adapterId);
            }
        });
    }

    public updateKnownDevices(devices: HermesDevice[]) {
        for (const device of devices) {
            if (this._deviceMap.has(device.port)) {
                continue;
            }

            const targetData: ITarget = {
                description: '',
                devtoolsFrontendUrl: '',
                faviconUrl: '',
                id: '',
                title: '',
                type: '',
                url: `localhost:${device.port}`,
                webSocketDebuggerUrl: '',
                adapterType: '',
            };
            const target = this.targetFactory('', targetData);
            target.on('socketClosed', (id) => {
                this.targetClose(device.port);
            });
            target.directConnectTo(`localhost:${device.port}`);
            this._deviceMap.set(device.port, target);
        }
    }

    private getTargetsOnDevice(device: AndroidTarget, port: number): Promise<IAndroidDeviceTarget[]> {
        return promiseWithTimeout(
            new Promise<IAndroidDeviceTarget[]>((resolve, reject) => {
                device
                    .callTarget('Valdi.enumerateDebuggableDevices', {})
                    .then((result) => {
                        const targets: IAndroidDeviceTarget[] = result.map((device: any) => {
                            const targetData: IAndroidDeviceTarget = {
                                deviceId: String(device.id),
                                deviceName: `${this._id}_${port}_${device.id}`,
                                deviceOSVersion: device.protocol,
                                url: `localhost:${port}`,
                                version: device.version,
                            };
                            return targetData;
                        });
                        resolve(targets);
                    })
                    .catch((err) => {
                        console.error('Valdi.enumerateDebuggableDevices returned error ' + err.message);
                        reject(err);
                    });
            }),
            5000,
        )
            .then((targets: IAndroidDeviceTarget[]) => {
                return targets;
            })
            .catch((err) => {
                this.targetClose(port);
                return [];
            });
    }

    public getTargets(): Promise<ITarget[]> {
        debug(`hermesAdapter.getTargets`);
        const targetPromises: Promise<IAndroidDeviceTarget[]>[] = [];
        this._deviceMap.forEach((target, port) => targetPromises.push(this.getTargetsOnDevice(target, port)));
        return Promise.all(targetPromises)
            .then((targets) => targets.flat())
            .then((devices: IAndroidDeviceTarget[]) => {
                // Now start up all the adapters
                devices.forEach((d) => {
                    const adapterId = d.deviceName;
                    if (!this._adapters.has(adapterId)) {
                        const parts = d.url.split(':');
                        if (parts.length > 1) {
                            // Get the port that the ios proxy exe is forwarding for this device
                            const port = parseInt(parts[1], 10);

                            // Create a new adapter for this device and add it to our list
                            const adapter = new Adapter(
                                adapterId,
                                this._proxyUrl,
                                { port: port },
                                this.targetFactory,
                                () => {
                                    const targets: ITarget[] = devices.map((device) => {
                                        const fakeAndroidData = {
                                            appId: device.deviceId,
                                            description: '',
                                            devtoolsFrontendUrl: '',
                                            faviconUrl: '',
                                            id: device.deviceId,
                                            title: device.deviceId,
                                            type: 'javascript',
                                            url: device.url,
                                            webSocketDebuggerUrl: '',
                                            adapterType: 'android',
                                            metadata: device,
                                        };
                                        const target = this.setTargetInfo(fakeAndroidData, device);
                                        return target;
                                    });
                                    return targets;
                                },
                            );
                            adapter.start();
                            adapter.on('socketClosed', (id) => {
                                this.emit('socketClosed', id);
                                adapter.stop();
                                this._adapters.delete(adapterId);
                            });
                            this._adapters.set(adapterId, adapter);
                        }
                    }
                });
                return Promise.resolve(devices);
            })
            .then((devices: IAndroidDeviceTarget[]) => {
                // Now get the targets for each device adapter in our list
                return super.getTargets(devices);
            });
    }

    public connectTo(url: string, wsFrom: WebSocket): AndroidTarget | undefined {
        const target = super.connectTo(url, wsFrom);
        if (!target) {
            return undefined;
        }
        const version = (target.data.metadata as IAndroidDeviceTarget).version;
        this.setProtocolFor(version, target);
        return target;
    }

    private setProtocolFor(version: string, target: AndroidTarget): HermesProtocol {
        return new HermesProtocol(target, version);
    }
}
