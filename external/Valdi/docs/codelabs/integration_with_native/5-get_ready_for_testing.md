**TODO**: Adapt previous exercise view code for testing

# Get ready for testing
Before we move on to the actual integration, let's make sure that we're set up for testing.

## Identifier files
Identifier files, or `ids.yaml` are config files that we used to generate unique string identifiers that can be referenced to from TypeScript and native. They can be used for accessiblity but we're going to use them here to make it easier to find views for tests.

Create an `ids.yaml` file in the top level of your `hello_world` folder.

`touch src/valdi/hello_world/ids.yaml`

Let's add some ids to this file.

```yaml
ids:
  title:
    description: The title of the hello world page
```

## Native testing
If you're just here for the testing, you can jump straight to the codelab for native testing.

### [iOS native testing](./ios/5-ios_testing.md)
### [Android native testing](./android/5-android_testing.md)

### [Next >](./6-native_build_module.md)
