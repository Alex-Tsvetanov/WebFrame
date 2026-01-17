<template>
    <CompositeAttributes is-layout="false">
        <Rectangle id="container" right="2" r-style="{{ viewModel?.style }}" left="{{ viewModel?.left }}"/>
    </CompositeAttributes>

</template>

<class-mapping>
    <Rectangle ios="UIRectangleView" android="com.snap.valdi.test.RectangleView"/>
</class-mapping>

<style>
Rectangle {
    top: 3;
    bottom: 4;
}
</style>
<script lang="ts">
// no-op
</script>
