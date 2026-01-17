# Troubleshooting

## Installation

A good first step is to search the web for the exact error output you see from the command.

These are common errors and how to resolve them.

<details>
    <summary><code>nvm not found</code></summary>

It's very likely either:

- The post install instructions for `nvm` were missed, use `brew info nvm` and follow the instructions for updating your shell environment (e.g. `.zshrc`, `.bashrc`)

```
You should create NVM's working directory if it doesn't exist:
mkdir ~/.nvm

Add the following to your shell profile e.g. ~/.profile or ~/.zshrc:
export NVM_DIR="$HOME/.nvm"
[ -s "/opt/homebrew/opt/nvm/nvm.sh" ] && \. "/opt/homebrew/opt/nvm/nvm.sh"  # This loads nvm
[ -s "/opt/homebrew/opt/nvm/etc/bash_completion.d/nvm" ] && \. "/opt/homebrew/opt/nvm/etc/bash_completion.d/nvm"  # This loads nvm bash_completion
```

- The shell you are currently using has not loaded the updated profile, eihter - close the terminal shell and open a new one - `source` the updated profile (e.g. `source ~/.zshrc`)
</details>

<details>
    <summary><code>Swap Space</code></summary>

Bazel eats a lot of memory, if you see Java memory errors, you don't have enough swap space.

8GB should be enough for an Android build of the hello world app.

</details>
