<template>
    <LimitToViewportChildComponent is-layout="false">
        <View id="slotContainer">
            <Slot ref="{{ slotRef }}"/>
        </View>
    </LimitToViewportChildComponent>
</template>

<script lang="ts">
import { ElementRef } from "valdi_core/src/ElementRef";

export const slotRef = new ElementRef();

slotRef.setAttribute('width', 10);
slotRef.setAttribute('height', 10);
slotRef.setAttribute('limitToViewport', true);

</script>