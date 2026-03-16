---
description: Sync Obsidian documentation with code changes since last update
---

# Documentation Sync Workflow (/doc_sync)

This workflow identifies code changes since the last documentation sync and updates the relevant Obsidian notes.

## Steps

1. **Read Sync State**: Load the last synchronized commit from `.agents/state/doc_state.json`. If it doesn't exist, use the parent of the current HEAD or a user-provided fallback.
2. **Identify Changes**:
   - Run `git log <last_commit>..HEAD --oneline --reverse` to see the commit history.
   - Run `git diff --name-only <last_commit> HEAD` to list all changed files.
3. **Analyze Impact**:
   - For each changed code file (C++, Dart, CMake), identify which Obsidian notes in `doc/obsidian/` are affected by the logic changes.
   - Account for merges by looking at the full diff between the last sync point and the current branch state.
4. **Update Documentation**:
   - For each affected note, update the content to reflect the new implementation.
   - Ensure wikilinks, callouts, and Mermaid diagrams are maintained or added.
   - Update the `last_updated` property in the note's frontmatter.
5. **Create New Notes**: If a new feature or architectural component was added, create a new note in the appropriate folder.
6. **Update Sync State**: Save the current HEAD commit hash to `.agents/state/doc_state.json`.

## Usage

Run this workflow after significant code changes or merges to ensure the documentation remains the source of truth for the project.

// turbo
7. Run `git rev-parse HEAD` to get the current hash and save it after verification.
