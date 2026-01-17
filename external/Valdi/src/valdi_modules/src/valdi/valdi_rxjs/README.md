# RxJS for Valdi components

This module is a mirror from npm package which contains files from `rx/src/internal`.
Files which are specific to the web platform were removed:
- `rx/src/internal/ajax`
- `rx/src/internal/observable/dom`
- `rx/src/internal/observable/bindNodeCallback.ts`

The `useDeprecatedSynchronousErrorHandling` code path has been removed and `errorContext` wrapping has been removed from `Observable.ts` and `Subject.ts` to ameliorate stack overflow problems on Android. 

Current version rxjs you can check in the `package.json`
