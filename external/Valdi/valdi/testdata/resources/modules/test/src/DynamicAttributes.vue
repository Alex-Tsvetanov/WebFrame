<template>
    <DynamicAttributes padding="{{ viewModel?.padding }}" is-layout="false">
        <Label color="red">{{ viewModel?.title }}</Label>
        <Label dummyHeight="{{ viewModel?.subtitleHeight }}"/>
    </DynamicAttributes>
</template>
<script lang="ts">
// no-op
</script>
