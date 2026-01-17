<template>
    <ForEachKeyItem is-layout="false">
        <Label>{{ this.idUsedOnCreate }}</Label>
        <Label>{{ viewModel.id }}</Label>
    </ForEachKeyItem>
</template>

<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

export interface ViewModel {
    id: string;
}

// @Component
export class MyComponent extends LegacyVueComponent<ViewModel> {

    idUsedOnCreate: string = '';

    onCreate() {
        this.idUsedOnCreate = this.viewModel.id;
    }

}
</script>
