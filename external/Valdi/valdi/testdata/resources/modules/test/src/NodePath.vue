<template>
    <NodePath is-layout="false">
        <View id="outer"/>
        <View id="container" for-each="{{ index of new Array(10) }}">
            <Label id="uniqueLabel"/>
            <Label id="multiLabel" for-each="{{ index of new Array(2) }}"/>
        </View>
    </NodePath>
</template>
<script lang="ts">
// no-op
</script>
