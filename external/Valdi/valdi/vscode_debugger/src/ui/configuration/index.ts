/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/

export * from './configurationProvider';
import {
  ChromeDebugConfigurationResolver,
  ChromeDebugConfigurationProvider,
} from './chromeDebugConfigurationProvider';
import {
  EdgeDebugConfigurationResolver,
  EdgeDebugConfigurationProvider,
} from './edgeDebugConfigurationProvider';
import { ExtensionHostConfigurationResolver } from './extensionHostConfigurationProvider';
import { NodeConfigurationResolver } from './nodeDebugConfigurationResolver';
import { ValdiConfigurationResolver } from './valdiDebugConfigurationResolver';
import { TerminalDebugConfigurationResolver } from './terminalDebugConfigurationResolver';
import {
  NodeInitialDebugConfigurationProvider,
  NodeDynamicDebugConfigurationProvider,
} from './nodeDebugConfigurationProvider';
import {
  ValdiInitialDebugConfigurationProvider,
  ValdiDynamicDebugConfigurationProvider,
} from './valdiDebugConfigurationProvider';

export const allConfigurationResolvers = [
  // ChromeDebugConfigurationResolver,
  // EdgeDebugConfigurationResolver,
  // ExtensionHostConfigurationResolver,
  // NodeConfigurationResolver,
  ValdiConfigurationResolver,
  // TerminalDebugConfigurationResolver,
];

export const allConfigurationProviders = [
  // ChromeDebugConfigurationProvider,
  // EdgeDebugConfigurationProvider,
  // NodeInitialDebugConfigurationProvider,
  // NodeDynamicDebugConfigurationProvider,
  // // TODO: Implement and enable these to display the list of devices we can attach to etc
  ValdiInitialDebugConfigurationProvider,
  // ValdiDynamicDebugConfigurationProvider,
];
