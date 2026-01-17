# Valdi Command Line References

The Valdi command line tools serve as the primary interface for setting up your environment, creating projects, building applications, and managing your Valdi workflow.

## Installation

```bash
npm install -g @snap/valdi
valdi dev_setup  # Sets up your entire development environment
```

For detailed installation instructions, see the [Installation Guide](../INSTALL.md).

> [!TIP]
> If you're contributing to Valdi itself, install from source instead:
> ```bash
> git clone git@github.com:Snapchat/Valdi.git
> cd Valdi/npm_modules/cli/
> npm run cli:install
> ```

## Basic Usage

A list of available commands can be displayed with `valdi --help`.

The `--help` option is also available within each individual command to display additional details.

```sh
valdi <command>

Commands:
  valdi completion                                             Generates completion script
  valdi bootstrap                                              Prepares and initializes a project to build and run in Valdi
  valdi dev_setup                                              Sets up the development environment for Valdi
  valdi projectsync [--target target]                          Synchronize VSCode project for the given targets
  valdi new_module [module-name]                               Create a new Valdi module with boilerplate structure
  valdi build <platform>                                       Build the application
  valdi install <platform>                                     Build and install the application to the connected device
  valdi package <platform>                                     Build and package a Valdi application
  valdi export <platform>                                      Build and export a Valdi library
  valdi hotreload [--module module_name] [--target             Starts the hotreloader for the application
  target_name]
  valdi test [--module module_name] [--target target_name]     Runs tests for given module(s) or target(s). Runs all
                                                               tests if no module or target is specified.
  valdi lint <command>                                         Checks and formats the code
  valdi doctor                                                 Check your Valdi development environment for common issues
  valdi log                                                    Streams Valdi logs from the connected device to console

Options:
  --debug    Run with debug logging                            [boolean] [default: false]
  --version  Show version number                               [boolean]
  --help     Show help                                         [boolean]
```

## Available CLI Commands

This section aims to cover each command more in-depth and will only provide some barebones usage patterns and descriptions since they are already available via the `--help` option.<br></br>

`valdi completion`\
Prints a code snippet and instructions to enable auto completion of available Valdi CLI commands. Optional and not required to use the CLI.<br></br>

`valdi bootstrap [-y --confirm-bootstrap] [-n --project-name]`\
Sets up a basic Valdi project in the current path by creating all required files to initially build and run the project.

Options:
- `-y, --confirm-bootstrap`: Skip confirmation prompt and start bootstrap process
- `-n, --project-name`: Name of the project
- `-t, --application-type`: Type of application to create
- `-l, --valdi-import`: Full path to a local checkout of the Valdi repo
- `-c, --with-cleanup`: Deletes all existing files in the current directory before initiating bootstrap<br></br>

`valdi dev_setup`\
Runs the automated environment setup script to install dependencies and configure the development environment. This command is typically run once during initial setup.<br></br>

`valdi projectsync [--target target]`\
Run this command to enable VS Code syntax highlighting for the current workspace project. This regenerates native bindings and updates VS Code project files.

- If `--target` is specified, synchronizes only the specified Bazel target.<br></br>

`valdi new_module [module-name]`\
Create a new Valdi module with boilerplate structure including BUILD.bazel, module.yaml, and basic source files.

Options:
- `module-name` (positional): Name of the Valdi module
- `--template`: Module template to use, will skip the prompt
- `--skip-checks`: Skips confirmation prompts

**Module naming requirements:**
- May contain: A-Z, a-z, 0-9, '-', '_', '.'
- Must start with a letter

Example:
```sh
# Interactive mode
valdi new_module

# Create module with options
valdi new_module my_module --skip-checks
```
<br></br>

`valdi build <platform> [--application] [--bazel_args]`\
Build the application without installing it to a device. Useful for CI/CD pipelines or when you just want to verify the build succeeds.

- Supported platforms: `android`, `ios`, `macos`, `cli`
- If no `--application` is specified, command will query for valid targets in the current workspace.
- Additional Bazel arguments can be passed in via `--bazel_args` option.<br></br>

`valdi install <platform> [--application] [--bazel_args]`\
Build and install a Valdi application on the given platform to a connected device or emulator.

- Supported platforms: `android`, `ios`, `macos`
- If no `--application` is specified, command will query for valid targets in the current workspace.
- Additional Bazel arguments can be passed in via `--bazel_args` option.<br></br>

`valdi package <platform> [--application] --output_path <path>`\
Build and package a Valdi application into a distributable format.

- Supported platforms: `android`, `ios`, `macos`
- `--output_path` (required): The path where to store the built application package
- `--application`: Name of the application to build

Example:
```sh
valdi package android --output_path ./dist/app.apk
valdi package ios --output_path ./dist/app.ipa
```
<br></br>

`valdi export <platform> [--library] --output_path <path>`\
Build and export a Valdi library for integration into native applications. Creates platform-specific library packages.

- Supported platforms: `android`, `ios`
- `--output_path` (required): The path where to store the exported library
  - iOS: Must end with `.xcframework`
  - Android: Must end with `.aar`
- `--library`: Name of the library to export (will query if not specified)

Example:
```sh
valdi export ios --output_path ./output/MyLibrary.xcframework
valdi export android --output_path ./output/mylibrary.aar
```
<br></br>

`valdi hotreload [--module module_name] [--target target_name]`\
Starts the Valdi [hotreloader](./start-about.md#prototype-quickly-with-hot-reload) for the application target.

- When `--module` and `--target` is omitted, it will attempt to run the application target of the current workspace. If there is more than one defined application, the specific target will need to be specified.
- The `--target` option should be a valid Bazel target (ex: `//:hello_world_hotreload`).
- The `--module` option will query and run targets in the current workspace which match the module_name.<br></br>

`valdi test [--module module_name] [--target target_name]`\
Executes the test(s) for the provided targets. Note that multiple modules OR targets can be provided to execute all tests simultaneously. If no modules or targets are provided, ALL tests within the current workspace will be ran.<br></br>

`valdi lint check [files...]`\
`valdi lint format [files...]`\
Runs the prettier linter on the provided files. The files given can be passed via wildcards. If a prettier config does not exist, a basic one will be created. Further customizations can be done via manually editing the created `.prettierrc.json`.<br></br>

`valdi doctor [--verbose] [--fix] [--json] [--framework] [--project]`\
Check your Valdi development environment for common issues. Performs comprehensive health checks on system requirements, tool installations, and workspace configuration.

Options:
- `--verbose, -v`: Show detailed diagnostic information
- `--fix, -f`: Attempt to automatically fix issues where possible
- `--json, -j`: Output results in JSON format (useful for CI/CD)
- `--framework, -F`: Include framework development checks (git-lfs, temurin, etc.)
- `--project, -p`: Include project-specific checks (workspace structure, etc.)

Example:
```sh
# Basic health check
valdi doctor

# With verbose output and automatic fixes
valdi doctor --verbose --fix

# Include all checks with JSON output
valdi doctor --project --framework --json
```

> [!TIP]
> Run `valdi doctor` if you encounter setup issues or build failures. It will identify common problems and suggest solutions.<br></br>

`valdi log`\
Displays and streams the Valdi logs. Will prompt user for selection if multiple logs are detected. The list of logs are ordered by descending modification time, with newest entries on the top.

> [!NOTE]
> Valdi logs are additionally printed to Logcat as well as the XCode console.
