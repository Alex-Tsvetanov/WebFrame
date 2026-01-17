<template>
    <NestedSlotsChild is-layout="false">
        <View id="beforeNestedSlot"/>
        <Slot/>
        <View id="afterNestedSlot"/>
    </NestedSlotsChild>
</template>

<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class MyComponent extends LegacyVueComponent {}
</script>
