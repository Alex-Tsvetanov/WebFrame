# Valdi Companion

This is the source for the Valdi Companion. It's written in TypeScript.

## Development

Install the necessary dependencies

```sh
npm install
```

Run the app without compilation

```sh
npm run main
```

Bundle into single JS file

```sh
npm run build
```

Bundle for development and use compiler with development build

```sh
# Bundle into dev package
npm run build-dev
# Build valdi_core with Bazel and instruct Bazel to use our previously built dev package
bzl build valdi_modules/src/valdi/valdi_core --//src/valdi/compiler/companion:dev_mode
```

## Testing

Tests are written using Jest

```sh
npm run test
```

or to enter the watch more:

```sh
npm run test-watch
```

## Code-style

We use [prettier](https://prettier.io/), to reformat all code:

```sh
npm run format
```
