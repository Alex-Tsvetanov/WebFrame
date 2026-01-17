<template>
    <TestImport is-layout="false" padding="{{ viewModel.padding }}">
        <Label color="red">{{ viewModel.title }}</Label>
        <Label dummyHeight="{{ viewModel.subtitleHeight }}"/>
    </TestImport>
</template>
<script lang="ts">
// no-op
</script>
