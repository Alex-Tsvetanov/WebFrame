# valdi-vivaldi

This is a valdi VS Code extension.

Right now it provides an easy way to show Valdi device logs right within VS Code when the hot reloader is running.

# Build for development

* Have VS Code installed
* Run `npm install`
* Open `valdi-vivaldi` in VS Code

# Package and install extension

* Run `npm run package-extension` to build and output the packaged .vsix file to `build/valdi-vivaldi.vsix`.
* Install the `.vsix` by running `code --install-extension build/valdi-vivaldi.vsix`

When working on this module please set your npm registry

```
npm config set registry https://registry.npmjs.org/
```

