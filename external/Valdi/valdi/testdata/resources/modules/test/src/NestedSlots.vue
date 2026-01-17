<template>
    <NestedSlots is-layout="false">
        <NestedSlotsParent is-layout="false">
            <View id="deepInside"/>
        </NestedSlotsParent>
    </NestedSlots>
</template>

<script lang="ts">
import { LegacyVueComponent } from 'valdi_core/src/Valdi';

// @Component
export class MyComponent extends LegacyVueComponent {}
</script>
