<template>
    <JSCallback is-layout="false">
        <View on-tap="{{ () => this.subViewTapped() }}"/>
        <View id="subView"/>
    </JSCallback>
</template>

<actions>
    <javascript name="increment"/>
</actions>

<script lang="ts">

import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class MyView extends LegacyVueComponent {

    sequence: number = 0;

    subViewTapped() {
        this.elements.subView.attributes.color = 'red';
        this.render();
    }

    increment(args: any) {
        this.sequence += 1;
        args.callback({
            sequence: this.sequence,
        });
    }

    incrementWithOptionalInvocation(args: any) {
        this.sequence += 1;
        args.callback?.({
            sequence: this.sequence,
        });
    }

};

</script>
