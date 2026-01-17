<template>
    <LimitToViewportChildOfChildParent is-layout="false">
        <LimitToViewportChildOfChild width="10" height="10" for-each="{{ child of viewModel.children }}" custom-tag="{{ child }}"/>
    </LimitToViewportChildOfChildParent>
</template>
<script lang="ts">
// no-op
</script>
