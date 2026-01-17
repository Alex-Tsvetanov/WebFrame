<template>
    <MainThread is-layout="false">
        <View height="{{ this.renderCount }}"/>
    </MainThread>
</template>

<script lang="ts">

import { LegacyVueComponent } from 'valdi_core/src/Valdi';
import { ValdiRuntime } from "valdi_core/src/ValdiRuntime"
import { makeMainThreadCallback } from 'valdi_core/src/utils/FunctionUtils';

declare var runtime: ValdiRuntime;

// @Component
export class MyView extends LegacyVueComponent {

    renderCount = 0;

    getRenders(completion: (any: any) => void): void {
        completion({
            // When called by native code, this render will synchronously
            // update the view tree in the same runloop.
            sync: makeMainThreadCallback(this.doRender.bind(this)),
            // This render will update the view tree in the next runloop.
            async: this.doRender.bind(this)
        });
    }

    doRender(): string {
        this.renderCount++;
        this.render();

        return 'This was synchronous';
    }
};

</script>
