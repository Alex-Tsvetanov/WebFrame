<template>
    <SlotRenderIfChild is-layout="false">
        <View id="container" render-if="{{ viewModel && viewModel.renderSlot }}">
            <View/>
            <Slot/>
            <View/>
        </View>
    </SlotRenderIfChild>
</template>
<script lang="ts">
// no-op
</script>
