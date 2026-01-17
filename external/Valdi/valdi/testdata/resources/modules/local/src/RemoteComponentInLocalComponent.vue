<template>
    <RemoteComponentInLocalComponent is-layout="false">
        <View>
            <Label>This part is local</Label>
        </View>
        <View>
            <Label>This part is remote</Label>
            <RemoteComponent/>
        </View>
    </RemoteComponentInLocalComponent>
</template>
<script lang="ts">
// no-op
</script>
