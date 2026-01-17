# Valdi CLI

The Valdi CLI tool provides helpful commands for setting up your environment, creating projects, building applications, and managing your Valdi workflow.

## For Users

The CLI is published to npm as `@snap/valdi`:

```bash
# Install globally
npm install -g @snap/valdi

# Set up your development environment
valdi dev_setup

# Verify your setup
valdi doctor

# Get help
valdi --help
```

### Create Your First App

After setting up your environment, create a new Valdi project:

```bash
# Create a new directory for your project
mkdir my_valdi_app
cd my_valdi_app

# Initialize the project
valdi bootstrap

# Build and install on iOS
valdi install ios

# Or build and install on Android
valdi install android

# Start hot reload for development
valdi hotreload
```

Now you can edit your TypeScript files and see changes instantly on your device!

For complete documentation, see:
- [Command Line Reference](https://github.com/Snapchat/Valdi/blob/main/docs/docs/command-line-references.md)
- [Installation Guide](https://github.com/Snapchat/Valdi/blob/main/docs/INSTALL.md)

### Key Commands

**`valdi dev_setup`** - Automated environment setup
- Installs all required dependencies (Bazel, Node.js, Java JDK 17, Android SDK, etc.)
- Configures environment variables and PATH
- Initializes Git LFS
- Sets up shell autocomplete
- Platform-specific: macOS (Homebrew, Xcode) or Linux (apt packages)

**`valdi doctor`** - Environment diagnostics
- Validates Node.js, Bazel, Java, Android SDK installations
- Checks Git LFS initialization
- Verifies shell autocomplete configuration
- Checks VSCode/Cursor extensions (warns if missing)
- macOS: Validates Xcode installation
- Supports `--framework` mode for additional checks
- Supports `--fix` to auto-repair issues
- Supports `--json` for CI/CD integration

**`valdi bootstrap`** - Project initialization
- Creates a new Valdi project in the current directory
- Sets up BUILD.bazel, WORKSPACE, package.json, and source files
- Supports multiple application templates (Hello World, Counter, etc.)

**`valdi install <platform>`** - Build and install
- Builds and installs app to connected device/simulator
- Platforms: `ios`, `android`, `macos`

**`valdi hotreload`** - Development server
- Enables instant hot reload during development
- Watches for file changes and updates app in milliseconds

For complete command documentation, see [Command Line Reference](https://github.com/Snapchat/Valdi/blob/main/docs/docs/command-line-references.md).

### Creating New Modules

```sh
valdi new_module

# Create module without prompts
valdi new_module my_new_module --skip-checks --android-class-path='com.example.my_module' --ios-module-name='XYZMyModule'

# Help
$ valdi new_module --help
valdi new_module [module-name]


******************************************
Valdi Module Creation Guide
******************************************

Requirements for Valdi module names:
- May contain: A-Z, a-z, 0-9, '-', '_', '.'
- Must start with a letter.

Recommended Directory Structure:
my_application/          # Root directory of your application
├── WORKSPACE            # Bazel Workspace
├── BUILD.bazel          # Bazel build
└── modules/
    ├── module_a/
    │   ├── BUILD.bazel
    │   ├── android/     # Native Android sources
    │   ├── ios/         # Native iOS sources
    │   ├── cpp/         # Native C++ sources
    │   └── src/         # Valdi sources
    │       └── ModuleAComponent.tsx
    ├── module_b/
        ├── BUILD.bazel
    │   ├── res/         # Image and font resources
    │   ├── strings/     # Localizable strings
        └── src/
            └── ModuleBComponent.tsx

For more comprehensive details, refer to the core-module documentation:
https://github.com/Snapchat/Valdi/blob/main/docs/docs/core-module.md

******************************************


Positionals:
  module-name  Name of the Valdi module.

Options:
  --debug               Run with debug logging                                                                                                  [boolean] [default: false]
  --version             Show version number                                                                                                                      [boolean]
  --help                Show help                                                                                                                                [boolean]
  --skip-checks         Skips confirmation prompts.                                                                                                              [boolean]
  --android-class-path  Android class path to use for generated Android sources.                                                                                  [string]
  --ios-module-name     iOS class prefix to use for generated iOS sources.                                                                                        [string]
```

## For Contributors

This section is for developers working on the Valdi CLI itself.

### Prerequisites

Set your npm registry when working on this module:

```sh
npm config set registry https://registry.npmjs.org/
```

### Development Setup

Install dependencies:

```sh
npm install
```

### Development

Run the CLI:

```sh
npm run main
```

# Pass in command line arguments

```sh
npm run main bootstrap -- --confirm-bootstrap
```

Build JavaScript output to `./dist`:

```sh
npm run build
```

Develop with hot reload:

```sh
npm run watch
node ./dist/index.js
node ./dist/index.js bootstrap --confirm-bootstrap
```

Show the help menu:

```sh
node ./dist/index.js new_module --help
```

Run unit tests:

```sh
npm test
```

Install the `valdi` command locally:

```sh
npm run cli:install
```
