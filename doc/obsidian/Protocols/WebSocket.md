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

## Implementation Details

### 1. Upgrade Detection
A WebSocket connection begins with an HTTP GET request containing specific headers:
- `Upgrade: websocket`
- `Connection: Upgrade`
- `Sec-WebSocket-Key`: A base64-encoded random key.
- `Sec-WebSocket-Version: 13`

Coroute's runtime detects these headers and switches the request handling path to the WebSocket logic before the ordinary router match completes if `App::ws` is being used.

### 2. Handshake & Accept Key
To complete the handshake, the server must calculate an "Accept Key":
1. Append the magic GUID `258EAFA5-E914-47DA-95CA-C5AB0DC85B11` to the client's `Sec-WebSocket-Key`.
2. Compute the **SHA-1** hash of the resulting string.
3. Base64-encode the hash.

The server then returns this in the `Sec-WebSocket-Accept` header with a `101 Switching Protocols` status.

### 3. Frame Model
WebSocket communication is broken into frames. Coroute supports:
- **Data Frames**:
    - `Text`: UTF-8 encoded text data.
    - `Binary`: Raw byte sequences.
- **Control Frames**:
    - `Close`: Initiates a graceful shutdown of the WebSocket session.
    - `Ping`: Sent by one side to check if the peer is still responsive.
    - `Pong`: The required response to a Ping.

### 4. Asynchronous Lifecycle
Like HTTP requests, the entire WebSocket lifecycle is asynchronous. The handler is a `Task<void>` that runs until the connection is closed.

```cpp
app.ws("/chat", [](auto ws) -> Task<void> {
    while (true) {
        auto msg = co_await ws->receive();
        if (!msg) break;
        if (msg->type == WebSocketMessage::Text) {
            co_await ws->send("Echo: " + msg->data);
        }
    }
});
```

Relevant files:
- `[[include/coroute/net/websocket.hpp]]`
- `[[src/net/websocket.cpp]]`
- `[[src/core/app.cpp]]`

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
