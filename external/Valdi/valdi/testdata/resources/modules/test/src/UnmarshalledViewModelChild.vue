<template is-layout="false">
    <UnmarshalledViewModelChild id="child"/>
</template>

<script lang="ts">

import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class MyComponent extends LegacyVueComponent {

    getCount(callback: (result: number) => void) {
        callback(this.viewModel.count as number);
    }

}

</script>
