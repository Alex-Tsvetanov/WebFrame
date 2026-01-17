<template>
    <NestedSlotsParent is-layout="false">
        <View id="first"/>
        <NestedSlotsChild is-layout="false">
            <Slot/>
        </NestedSlotsChild>
        <View id="last"/>
    </NestedSlotsParent>
</template>

<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class MyComponent extends LegacyVueComponent {}
</script>
