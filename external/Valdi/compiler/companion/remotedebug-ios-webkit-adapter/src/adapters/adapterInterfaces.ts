//
// Copyright (C) Microsoft. All rights reserved.
//

interface IDeviceTarget {
    deviceId: string;
    deviceName: string;
    deviceOSVersion: string;
    url: string;
    version: string;
}
export interface ITarget {
    appId?: string;
    description: string;
    devtoolsFrontendUrl: string;
    faviconUrl: string;
    id: string;
    title: string;
    type: string;
    url: string;
    webSocketDebuggerUrl: string;
    adapterType: string;
    metadata?: IDeviceTarget;
}

export interface IAdapterOptions {
    pollingInterval?: number;
    baseUrl?: string;
    path?: string;
    port?: number;
    proxyExePath?: string;
    proxyExeArgsProvider?: () => Promise<string[]>;
}

export interface IIOSDeviceTarget extends IDeviceTarget {}

export interface IIOSProxySettings {
    proxyPath: string;
    proxyPort: number;
    proxyExeArgsProvider: () => Promise<string[]>;
}

export interface IAndroidDeviceTarget extends IDeviceTarget {}
