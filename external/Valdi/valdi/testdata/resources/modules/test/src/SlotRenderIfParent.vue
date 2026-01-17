<template>
    <SlotRenderIfParent is-layout="false">
        <SlotRenderIfChild view-model="{{ viewModel && viewModel.childViewModel }}">
            <View id="child"/>
        </SlotRenderIfChild>
    </SlotRenderIfParent>
</template>
<script lang="ts">
// no-op
</script>
