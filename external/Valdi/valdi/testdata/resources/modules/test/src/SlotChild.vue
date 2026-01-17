<template>
    <SlotChild is-layout="false">
        <View id="top"/>
        <View id="header-container">
            <Slot name="header" ref="{{ headerRef }}"/>
        </View>
        <Slot/>
        <View id="inside" render-if="{{ viewModel && viewModel.renderInside }}"/>
        <Slot name="after-body"/>
        <View id="footer-container">
            <Slot name="footer" ref="{{ footerRef }}"/>
        </View>
    </SlotChild>
</template>

<script>
import { ElementRef } from "valdi_core/src/ElementRef";

export const footerRef = new ElementRef();
footerRef.setAttribute('color', '#00ff00');

export const headerRef = new ElementRef();
headerRef.setAttribute('color', '#ff0000');

</script>