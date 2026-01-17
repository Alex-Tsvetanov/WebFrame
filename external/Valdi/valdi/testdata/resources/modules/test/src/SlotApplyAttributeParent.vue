<template>
    <SlotApplyAttributeParent is-layout="false">
        <SlotApplyAttribute :value-in-slot="viewModel.valueInSlot">
            <Label id="myChild" for-each="{{ value of [0, 1] }}">
                {{ 'Item ' + value }}
            </Label>
        </SlotApplyAttribute>
    </SlotApplyAttributeParent>
</template>
<script lang="ts">
// no-op
</script>
