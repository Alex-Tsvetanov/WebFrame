import { Component } from "valdi_core/src/Component";
import { RequireFunc } from "valdi_core/src/IModuleLoader";

declare const require: RequireFunc;

interface NativeModuleReferenceTest {
    makeCallback1(): () => any;
    makeCallback2(): () => any;
}

const nativeModule: NativeModuleReferenceTest = require('NativeModuleReferenceTest');

let callback1: (() => any) | undefined;
let callback2: (() => any) | undefined;

export class NativeModuleReference extends Component {

    onCreate() {
        // We emit those native callbacks under the NativeModuleReference's Context
        // which we store as global so that we can later call them in the global Context
        callback1 = nativeModule.makeCallback1();
        callback2 = nativeModule.makeCallback2();
    }
}

export function callCallback1(): any {
    return callback1!();
}

export function callCallback2(): any {
    return callback2!();
}
