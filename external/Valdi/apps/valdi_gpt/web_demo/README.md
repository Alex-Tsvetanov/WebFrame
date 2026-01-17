# Valdi GPT web demo

This demo app is for the ongoing development of Valdi Web.

## Build the web dependencies

```
bazel build //apps/valdi_gpt:valdi_gpt_npm
```

```
cd bazel-bin/apps/valdi_gpt/valdi_gpt_npm 
npm link
```

```
cd apps/valdi_gpt/web_demo
npm install
npm link valdi_gpt_npm
```

`link` has to be run after install because it modifies the `node_modules` folder

## Run the dev app

From the `web_demo` directory.

```
npm run serve
```

Open up `http://localhost:3030/` in a web browser.

The app should hotreload.
