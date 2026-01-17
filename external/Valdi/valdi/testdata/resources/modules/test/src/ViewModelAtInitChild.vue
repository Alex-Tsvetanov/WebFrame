<template is-layout="false">
    <ViewModelAtInitChild>
    </ViewModelAtInitChild>
</template>


<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class ViewModelAtInitChild extends LegacyVueComponent {

    hadViewModelOnCreate = false;

    hadViewModel(callback: (v: boolean) => void) {
        callback(this.hadViewModelOnCreate);
    }

    onCreate() {
        this.hadViewModelOnCreate = !!this.viewModel;
    }

}
</script>
