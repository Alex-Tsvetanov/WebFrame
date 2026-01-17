import { RequireFunc } from 'valdi_core/src/IModuleLoader';
import { ValdiProtobufModule } from './ValdiProtobuf';

declare const require: RequireFunc;

let protobufModule: ValdiProtobufModule | undefined;

export function setProtobufModule(module: ValdiProtobufModule | undefined) {
  protobufModule = module;
}

export function getProtobufModule(): ValdiProtobufModule {
  if (!protobufModule) {
    protobufModule = require('ValdiProtobuf', true) as ValdiProtobufModule;
  }

  return protobufModule;
}
