<template>
    <ComponentInsideSlot is-layout="false">
        <SlotRenderIfChild :render-slot="viewModel.renderChild">
            <JSInstance/>
        </SlotRenderIfChild>
    </ComponentInsideSlot>
</template>
<script lang="ts">
// no-op
</script>
