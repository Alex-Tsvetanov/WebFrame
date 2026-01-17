## Jasmine for valdi components

This module is a mirror from npm package which contains files from `jasmine-core/lib/jasmine-core`.
Current version `jasmine-core` and `@types/jasmine` you can check in the `package.json`.
This mirror includes minor changes for testing Valdi modules TypeScript sources. 

Steps to update:
- copy from `jasmine/node_modules/@types/jasmine/index.d.ts` to `jasmine/src/jasmine.d.ts`
- copy from `jasmine-core/lib/jasmine-core/jasmine.js` to `jasmine/src/jasmine.js`

>  Then you need to update parts of logic in `jasmine.js` 
to make sure that jasmine stack trace has the right reference for the error. 

```
    stackTrace.frames.forEach(function (frame) {
-        if (frame.file && frame.file !== jasmineFile) {
+        if (frame.file && frame.file === jasmineFile) {
+          if (result[result.length - 1] !== jasmineMarker) {
+            result.push(jasmineMarker);
+          }
+        } else {
           result.push(frame.raw);
-        } else if (result[result.length - 1] !== jasmineMarker) {
-          result.push(jasmineMarker);
         }
       });
 
@@ -7643,10 +7645,10 @@ getJasmineRequireObj().StackTrace = function (j$) {
         }
 
         fileLineColMatch = overallMatch[pattern.fileLineColIx].match(
-          /^(.*):(\d+):\d+$/
+          /^(.*):(\d+)(?::\d+)?$/
         );
         if (!fileLineColMatch) {
-          return null;
+          return { raw: line };
         }
 
         style = style || pattern.style;

```

- copy from `jasmine-core/lib/jasmine-core/node_boot.js` to `jasmine/src/origin_boot.js`

> please make sure that there is no node.js related API used.


`boot.js` contains Valdi related integration files.
