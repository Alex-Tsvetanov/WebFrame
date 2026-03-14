---
trigger: glob
globs: **/*.tex,**/*.md
description: Documentation Language Guidelines
---

# Documentation Language Guidelines

When generating, updating, or maintaining any project documentation, the following rules MUST be strictly followed regarding language:

1. **Bilingual Requirement**: All core documentation (e.g., `doc/v2/` chapters, abstracts, conclusions) must be provided in both **English (EN)** and **Bulgarian (BG)**.
2. **Directory Structure**:
   - Bulgarian documentation should be placed in `doc/v2/bg/` (or suffixed appropriately if in the same directory, e.g., `00_abstract_bg.tex`).
   - English documentation should be placed in `doc/v2/en/` (or suffixed appropriately, e.g., `00_abstract_en.tex`).
   - If a unified document is used, sections must clearly delineate the languages, or separate PDFs should be generated.
3. **Synchronization**: Whenever a change is made to the Bulgarian documentation, an equivalent and translation-accurate change MUST be made to the English documentation, and vice-versa.
4. **Style**: Both languages must maintain the formal, scientific writing style required by the project.

Always assume that new chapters or architectural explanations need to be translated or generated in both languages simultaneously.
