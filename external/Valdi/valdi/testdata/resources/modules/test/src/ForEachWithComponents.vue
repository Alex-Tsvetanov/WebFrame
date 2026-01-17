<template>
    <ForEachWithComponents is-layout="false">
        <JSInstance for-each="{{ instance of viewModel.children }}"/>
    </ForEachWithComponents>
</template>
<script lang="ts">
// no-op
</script>
