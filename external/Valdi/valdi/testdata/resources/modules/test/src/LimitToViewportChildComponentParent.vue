<template>
    <LimitToViewportChildComponentParent is-layout="false">
        <LimitToViewportChildComponent>
            <JSInstance for-each="{{ index of [0, 1, 2, 3] }}"/>
        </LimitToViewportChildComponent>
    </LimitToViewportChildComponentParent>
</template>
<script lang="ts">
// no-op
</script>
