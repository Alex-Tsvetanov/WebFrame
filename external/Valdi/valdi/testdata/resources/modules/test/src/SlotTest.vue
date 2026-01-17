<template>
    <SlotTest is-layout="false">
        <View/>
        <SlotChild view-model="{{ viewModel ? viewModel.slotViewModel : undefined }}">
            <View slot="header" render-if="{{ viewModel && viewModel.renderHeader }}" color="#0000ff"/>
            <Label for-each="{{ label of (viewModel ? viewModel.labels : []) }}">{{ label }}</Label>
            <View slot="footer"/>
        </SlotChild>
    </SlotTest>
</template>
<script lang="ts">
// no-op
</script>
