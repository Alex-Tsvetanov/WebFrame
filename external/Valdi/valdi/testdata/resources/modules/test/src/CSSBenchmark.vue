
<template>
  <CSSDemoView
    id="root"
    class="main"
  >
    <ScrollView>
      <View class="section">
        <Label class="title">Dynamic CSS</Label>
        <Label class="desc">Click "toggle button state" to toggle the class on a View, which changes the CSS selectors
          its children are styled by.</Label>
        <View
          id="button"
          class="disabled"
        ><Label>A Button!!!</Label></View>
        <Button
          id="toggleButton"
        >Toggle Button State</Button>
      </View>

      <View class="section">
        <Label class="title">Styled text</Label>
        <Label id="text-demo">Some label</Label>
      </View>

      <View class="section gradient-test">
        <Label>This is a linear gradient test</Label>
      </View>

      <View class="section">
        <Label class="title">First/last child</Label>
        <Label class="desc">We can style elements via :first-child and :last-child. At the bottom of this example view,
          there"s a node rendered via for-each that returns zero items, so the node before it becomes the last
          child.</Label>
        <View class="first_last_container">
          <Label>I am the first child.</Label>
          <Label>I am the middle child.</Label>
          <Label>I am the last child!</Label>
        </View>
      </View>

      <View class="section">
        <Label class="title">CSS ancestry</Label>
        <Label class="desc">This demo shows use of the direct descendent selector.</Label>
        <View class="ancestry">
          <Label>I am the first level.</Label>
          <View>
            <Label>I am the second level.</Label>
            <View>
              <Label>I am the third level deep!</Label>
            </View>
          </View>
        </View>
      </View>

    </ScrollView>
  </CSSDemoView>
</template>
<class-mapping>
  <Button ios="UIButton" android="com.snap.valdi.views.ValdiButton" />
</class-mapping>

<style>
  Label {
    font-family: Avenir Next;
    font-weight: demi-bold;
  }
.main {
  background-color: #010103;
  flex-direction: column;
  align-items: stretch;
  justify-content: flex-start;
}
.section {
  background-color: #ffffff;
  margin: 20;
  border-radius: 10;
  padding: 15;
  margin-top: 0;
}
.section:first-child {
  margin-top: 20;
}
.section > * {
  align-self: flex-start;
}
.title {
  font-size: 16;
}
.desc {
  font-size: 12;
  font-weight: medium;
  margin-bottom: 14;
  margin-top: 4;
  number-of-lines: 0;
  opacity: 0.5;
}
#button {
  background-color: rgba(0,25,51,0.05);
  padding: 10;
  padding-left: 15;
  padding-right: 15;
  border: 1 solid rgba(0,0,0,0.1);
  box-shadow: 0px 2px 10 rgba(0, 0, 0, 0.2);
}
#button Label {
  font-size: 16;
}
#button.enabled Label {
  color: #220000;
}
#button.disabled Label {
  color: #777777;
}
#toggleButton {
  color: rgba(0,0,0,0.5);
  font-size: 12;
  margin-top: 10;
  padding: 10;
  padding-left: 0;
  text-decoration: underline;
}
.first_last_container {
  align-self: stretch;
  flex-direction: row;
  justify-content: space-between;
  background-color: #eeeeee;
}
.first_last_container Label {
  font-size: 12;
  margin: 5;
  color: #449944;
  flex-shrink: 1;
  number-of-lines: 0;
  text-align: center;
}
.first_last_container Label:first-child {
  color: #ff5555;
}
.first_last_container Label:last-child {
  color: #444499;
}
.ancestry View {
  margin-left: 5;
}
.ancestry Label {
  font-size: 12;
}
.ancestry > Label {
  color: #ff5555;
}
.ancestry > View > Label {
  color: #449944;

}
.ancestry > View > View > Label {
  color: #444499;
}
#text-demo {
  line-height: 2;
  number-of-lines: 0;
  font-weight: light;
  text-align: center;
}
.gradient-test {
  background: linear-gradient(rgba(0, 0, 0, 0.5), rgba(255, 0, 0, 0.5), rgba(0, 255, 0, 0.5));
}
</style>
<script lang="ts">
// no-op
</script>
