<template is-layout="false">
    <ViewModelAtInitParent>
        <ViewModelAtInitChild view-model="{{ {} }}"/>
    </ViewModelAtInitParent>
</template>

<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class ViewModelAtInitParent extends LegacyVueComponent {

    hadViewModelOnCreate = false;

    hadViewModel(callback: (v: boolean) => void) {
        callback(this.hadViewModelOnCreate);
    }

    onCreate() {
        this.hadViewModelOnCreate = !!this.viewModel;
    }

}
</script>
