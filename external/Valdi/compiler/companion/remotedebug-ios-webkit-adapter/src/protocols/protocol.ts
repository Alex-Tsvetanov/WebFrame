//
// Copyright (C) Microsoft. All rights reserved.
//

import { AdapterTarget } from './target';

export class ProtocolAdapter {
    protected _target: AdapterTarget;

    constructor(target: AdapterTarget) {
        this._target = target;
    }
}
