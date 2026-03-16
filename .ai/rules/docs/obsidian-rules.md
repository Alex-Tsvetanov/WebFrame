---
trigger: glob
globs: doc/obsidian/**/*.md
description: Obsidian Documentation Style and Structure
---

# Obsidian Documentation Rules

When creating or updating notes in the Obsidian vault (`doc/obsidian/`), follow these rules:

1. **Vault Structure**: Maintain the folder hierarchy (Architecture, Protocols, Renderers, etc.). If a new category is needed, create a corresponding folder.
2. **Wikilinks**: Use `[[Note Name]]` for internal linking between documents. Avoid standard markdown links for internal vault references.
3. **Callouts**: Use Obsidian callouts (e.g., `> [!INFO]`, `> [!WARNING]`) to highlight important information, implementation details, or architectural caveats.
4. **Metadata**: Each note should include frontmatter with at least `tags` and `last_updated`.
5. **Atomic Notes**: Favor smaller, atomic notes that focus on a single concept rather than massive monolithic files.
6. **Diagrams**: Use Mermaid diagrams within notes to visualize flows, relationships, or state machines.

## Synchronization Requirement

Every functional code change must propagate to the relevant Obsidian notes. Use the `/doc_sync` workflow to ensure nothing is missed after complex changes or merges.
