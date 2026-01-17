<template>
    <ViewCallbacks is-layout="false">
        <View render-if="{{ viewModel && viewModel.shouldRender }}" on-view-create="{{ viewModel.onViewCreate }}" on-view-destroy="{{ viewModel.onViewDestroy }}" on-view-change="{{ viewModel.onViewChange }}"/>
    </ViewCallbacks>
</template>
<script lang="ts">
// no-op
</script>
