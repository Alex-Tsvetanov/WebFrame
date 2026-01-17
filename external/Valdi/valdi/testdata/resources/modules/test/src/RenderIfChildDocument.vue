<template>
    <RenderIfChildDocument is-layout="false">
        <View id="childContainer" render-if="{{ viewModel.renderChild }} ">
            <BasicViewTree/>
        </View>
    </RenderIfChildDocument>
</template>
<script lang="ts">
// no-op
</script>
