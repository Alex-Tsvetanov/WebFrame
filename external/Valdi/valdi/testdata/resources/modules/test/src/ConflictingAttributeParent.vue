<template>
    <ConflictingAttributeParent is-layout="false">
        <ConflictingAttributeChild color="{{ viewModel.color }}" class="{{ viewModel.class }}" :color="viewModel.childColor" :class="viewModel.childClass"/>
    </ConflictingAttributeParent>
</template>

<style>
.yellow {
    color: yellow;
}
</style>
<script lang="ts">
// no-op
</script>
