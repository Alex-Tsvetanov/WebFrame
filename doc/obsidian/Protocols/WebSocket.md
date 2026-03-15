---
title: WebSocket
tags:
  - coroute
  - websocket
  - protocol
aliases:
  - WebSocket Support
  - RFC 6455 Support
---

# WebSocket

> [!abstract]
> Coroute includes a WebSocket upgrade and session layer on top of the HTTP runtime. It exposes a `WebSocketConnection` abstraction, upgrade helpers, and `App::ws(...)` route registration for real-time application endpoints.

## Main abstractions

Relevant files:

- `[[include/coroute/net/websocket.hpp]]`
- `[[src/net/websocket.cpp]]`
- `[[include/coroute/core/app.hpp]]`

### `WebSocketConnection`

The runtime abstraction exposes:

- `receive()`
- `send_text(...)`
- `send_binary(...)`
- `ping(...)`
- `pong(...)`
- `close(...)`
- connection metadata accessors

### Message and frame model

The public header includes:

- frame opcodes
- close codes
- message helpers for text/binary/close/ping/pong interpretation
- upgrade helpers for RFC 6455 handshake processing

## Runtime integration

The application-level registration surface is:

- `App::ws(path, handler)`

At runtime, Coroute attempts to upgrade eligible HTTP requests into WebSocket sessions and then hands the connection to the registered handler.

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`
- `[[include/coroute/net/websocket.hpp]]`
- `[[src/net/websocket.cpp]]`

## Example usage in the repository

Dedicated examples and app usage sites include:

- `[[examples/Samples/websocket_server/main.cpp]]`
- `[[examples/Project/src/handlers/websocket/task_hub.cpp]]`
- `[[examples/FlutterProject/src/handlers/websocket/task_hub.cpp]]`

These examples show two patterns:

- simple echo/chat usage in samples
- application broadcast/update hubs in the larger example apps

## Relation to Flutter

WebSocket is not only a server protocol in this repository. It also supports the remote-update path for Flutter.

Observed flow:

- `GenericView` can connect to `Bridge.wsBaseUrl + /ws`
- example applications expose `/ws` endpoints through `TaskHub`-style handlers
- this complements the in-process broadcast callback path when Flutter is talking to a remote server instead of sharing a process with Coroute

Relevant files:

- `[[packages/Flutter/coroute_framework/lib/generic_view.dart]]`
- `[[examples/FlutterProject/src/handlers/websocket/task_hub.cpp]]`

## Features visible from the public header

From `[[include/coroute/net/websocket.hpp]]`, Coroute visibly supports:

- text and binary frames
- close, ping, and pong control frames
- configurable message and frame size limits
- allowed-origin and subprotocol configuration fields
- RFC 6455 close codes and opcodes

## Testing and implementation evidence

Implementation files are present for the protocol.

- `[[include/coroute/net/websocket.hpp]]`
- `[[src/net/websocket.cpp]]`

Examples are present.

- `[[examples/Samples/websocket_server/main.cpp]]`
- `[[examples/Project/src/handlers/websocket/task_hub.cpp]]`
- `[[examples/FlutterProject/src/handlers/websocket/task_hub.cpp]]`

During this pass, I did **not** find a dedicated test file in `[[tests]]` named specifically for WebSocket.

## Status

### Current status

- **Implemented**: upgrade helpers, connection abstraction, and app-level route registration surface are present.
- **Implemented**: sample and application usage sites exist.
- **Integrated**: repository examples use WebSocket for task/broadcast style updates.
- **Test note**: I did not find a dedicated WebSocket test file in this pass.

## Related notes

- [[Protocols/Protocols Index]]
- [[Architecture/Server Runtime and OS Backends]]
- [[Architecture/Flutter Integration]]
- [[Protocols/HTTP-1.1]]
