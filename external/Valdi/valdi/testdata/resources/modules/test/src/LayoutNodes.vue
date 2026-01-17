
<template>
    <LayoutNodes is-layout="false">
        <Layout margin-left="10" margin-top="20" margin-bottom="30">
            <Layout padding="2">
                <View id="outer">
                    <Label id="inner" margin-left="4" margin-right="4" width="10" height="10"/>
                </View>
            </Layout>
            <View id="lonely" width="5" height="5"/>
            <Layout padding="4">
                <Label id="innerLayout" width="10" height="10"/>
            </Layout>
        </Layout>
    </LayoutNodes>
</template>
<script lang="ts">
// no-op
</script>
