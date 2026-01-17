import { RequireFunc } from 'valdi_core/src/IModuleLoader';
import { FileSystemModule } from './FileSystemModule';

declare const require: RequireFunc;

export const VALDI_MODULES_ROOT = './valdi_modules/src/valdi';

export const fs = require('FileSystem') as FileSystemModule;
