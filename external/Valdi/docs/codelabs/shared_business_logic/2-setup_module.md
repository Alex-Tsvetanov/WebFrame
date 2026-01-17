# Setup your Module
The time has come to create your own unit of Valdi. Features are separated into modules.

## What is a module
Modules live in `src/valdi_modules/src/valdi/<component_name>` (relative to the Valdi root directory).

A module is a set of typescript classes, components, and functions. Modules may, but do not have to, contain image assets, localized strings, and TypeScript tests.

Modules can depend on other modules.

All of these things get compiled down into **valdimodule** files that get read by the **Valdi Runtime** and render natively on each platform.

## Create a new module
We have a convenient script to create a new module for you that sets up the basic structure and creates a few important files.

**TODO**: Provide correct parent directory/instructions to the valdi cli

`./scripts/new_module.py hello_world`

This creates a new module called `hello_world`.

Open VSCode and navigate to your code.

Your new code will be at `src/valdi/hello_world`

You should see a handful of files and folders:

- `module.yaml` - Dependencies and generated output files
- `tsconfig.json` - Typescript compilation options
- `strings.yaml` - Localized strings
- `res/` - Image assets
- `src/` - Typescript code
    - `HelloWorld.tsx` - Main entry component

The defaults in `module.yaml` and `tsconfig.json` will work fine for our purposes but feel free to check them out. 

This `HelloWorld.tsx` is the default starting point for UI Components but we won't need it. Go ahead and delete it.

Instead, create a new TypeScript file `Calculator.ts`. This should be a TypeScript file not a TSX file. Learn more about TSX in the [**Getting Started codelab**](../getting_started/5-declarative_rendering.md#tsxjsx).

### [Next >](./3-business_logic.md)
