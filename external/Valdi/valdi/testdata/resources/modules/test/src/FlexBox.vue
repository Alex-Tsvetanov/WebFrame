<template>
    <FlexBox is-layout="false">
        <View id="child1" render-if="{{ viewModel.renderChild1 }} "/>
        <View id="child2"/>
        <View id="child3" render-if="{{ viewModel.renderChild2 }} "/>
        <View id="child4"/>
    </FlexBox>
</template>
<script lang="ts">
// no-op
</script>
