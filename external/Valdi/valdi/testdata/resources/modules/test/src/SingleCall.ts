import { makeSingleCallCallback } from 'valdi_core/src/utils/FunctionUtils';

export function getCallback(): () => string {
    return () => 'Hello World';
}

export function callCallback(iteration: number, cb: () => void): void {
    for (let i = 0; i < iteration; i++) {
        cb();
    }
}

export function getSingleCallback(): () => string {
    return makeSingleCallCallback(() => 'Hello World');
}
