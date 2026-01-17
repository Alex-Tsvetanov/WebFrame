//
// Copyright (C) Microsoft. All rights reserved.
//

import * as WebSocket from 'ws';
import { debug } from '../logger';
import { AndroidTarget } from '../protocols/androidTarget';
import { IOSProtocol } from '../protocols/ios/ios';
import { IOS9Protocol } from '../protocols/ios/ios9';
import { Adapter } from './adapter';
import { AdapterCollection } from './adapterCollection';
import { IAndroidDeviceTarget, ITarget } from './adapterInterfaces';

export type AndroidDevice = { deviceId: string; endpoint: string };

export class AndroidAdapter extends AdapterCollection<AndroidTarget> {
    private _protocolMap: Map<AndroidTarget, IOSProtocol>;
    private _devices: AndroidDevice[] = [];

    constructor(id: string, socket: string) {
        super(id, socket, {}, (targetId, targetData) => new AndroidTarget(targetId, targetData));
        this._protocolMap = new Map<AndroidTarget, IOSProtocol>();
    }

    public updateKnownDevices(devices: AndroidDevice[]) {
        this._devices = devices;
    }

    public getTargets(): Promise<ITarget[]> {
        debug(`androidAdapter.getTargets`);

        return new Promise<IAndroidDeviceTarget[]>(resolve => {
            const targets = this._devices.map(device => {
                const target: IAndroidDeviceTarget = {
                    deviceId: device.deviceId,
                    deviceName: 'Android',
                    deviceOSVersion: 'Android 999',
                    url: device.endpoint,
                    version: '9.3.0',
                };
                return target;
            });
            resolve(targets);
        })
            .then((devices: IAndroidDeviceTarget[]) => {
                // Now start up all the adapters
                devices.forEach(d => {
                    const adapterId = `${this._id}_${d.deviceId}`;

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
                                    const targets: ITarget[] = devices.map(device => {
                                        const fakeAndroidData = {
                                            appId: device.deviceId,
                                            description: '',
                                            devtoolsFrontendUrl: '',
                                            faviconUrl: '',
                                            id: '1',
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
                            adapter.on('socketClosed', id => {
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

        if (!this._protocolMap.has(target)) {
            const version = (target.data.metadata as IAndroidDeviceTarget).version;
            const protocol = this.getProtocolFor(version, target);
            this._protocolMap.set(target, protocol);
        }
        return target;
    }

    private getProtocolFor(version: string, target: AndroidTarget): IOSProtocol {
        return new IOS9Protocol(target);
    }
}
