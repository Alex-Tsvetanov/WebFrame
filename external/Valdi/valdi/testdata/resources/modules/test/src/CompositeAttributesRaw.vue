<template>
    <CompositeAttributesRaw is-layout="false">
        <Rectangle rectangle="{{ viewModel.rectangle }}"/>
    </CompositeAttributesRaw>

</template>

<class-mapping>
    <Rectangle ios="UIRectangleView" android="com.snap.valdi.test.RectangleView"/>
</class-mapping>

<script lang="ts">
// no-op
</script>
