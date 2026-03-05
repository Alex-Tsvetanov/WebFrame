---
description: Review project codemap and architecture
---
# Review Project Codemap

This workflow should be run or referenced every time before starting a new request in the WebFrame project.

1. Read the `codemap.md` file located at the root of the project.
2. Review the directory structure and understand the core components (App, routing, middleware, etc.).
3. Identify the specific domains (e.g. `include/coroute/net`, `src/http2`) pertinent to the user's request.
4. Read the corresponding header interfaces before attempting any modifications.
5. Plan the implementation adhering to Coroute v2's conventions.
