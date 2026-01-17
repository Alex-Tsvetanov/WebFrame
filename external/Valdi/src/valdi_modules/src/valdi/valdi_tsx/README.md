# Valdi Vendors

## Changes

A list of changes made to the third party vendor files that will need to be replicated when updating.

### Protobuf.js

`Protobuf.js` has been modified to require `Long.js`, it’s important to include this change when updating `Protobuf.js`.

Search for `.configure =` to find the definition of configure, then go to that function and add the following line to the beginning of it:

```js
r.util.Long = Long
```

NOTE: `r` should match the var scope defined above it (as of this writing, it’s `var r = n`).

### NPM libraries

>  Note: the library files still have to be copied manually into `valdi_core/src` to be included in the valdi_modules compilation process.

`valdi_core` contains a copy of the [tslib library](https://github.com/microsoft/tslib).
The current version you can find in the `valdi_core/package.json`
To update the tslib library you need to copy two files:
- `valdi_core/node_modules/tslib/tslib.js` -> `valdi_core/src/tslib.js`
- `valdi_core/node_modules/tslib/tslib.d.ts` -> `valdi_core/src/tslib.d.ts`
