# Task Dashboard

A **real-time collaborative task management application** built with Coroute.

Teams can create, assign, and track tasks with instant updates across all connected clients. This project demonstrates production-level patterns for building web applications with the Coroute framework.

## Use Case

- **Project managers** create and assign tasks to team members
- **Team members** update task status (pending → in progress → completed)
- **Everyone** sees changes instantly via WebSocket - no page refresh needed
- **Dashboard** shows task statistics and recent activity

## Features

- **REST API** - Full CRUD for tasks and users
- **WebSocket** - Real-time task updates broadcast to all clients
- **Server-Side Rendering** - Jinja2-style templates with inja
- **Authentication** - Session-based login/logout
- **Middleware** - Request logging, authentication guards
- **Static Files** - CSS, JavaScript assets

## Quick Start

```bash
# From Coroute root directory
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target task_dashboard

# Run the server
./build/v2/examples/Project/task_dashboard
```

Open <http://localhost:8080> in your browser.

## Project Structure

```text
Project/
├── src/
│   ├── main.cpp              # Entry point
│   ├── app/                  # Server configuration
│   ├── middleware/           # Auth, logging
│   ├── handlers/             # Route handlers
│   │   ├── pages.cpp         # HTML pages
│   │   ├── api/              # REST endpoints
│   │   └── websocket/        # Real-time hub
│   ├── models/               # Data structures
│   └── services/             # Business logic
├── tests/                    # Unit & integration tests
├── templates/                # HTML templates
├── static/                   # CSS, JS
└── docs/                     # Architecture guide
```

## API Endpoints

### Tasks

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/tasks` | List all tasks |
| POST | `/api/tasks` | Create a task |
| GET | `/api/tasks/{id}` | Get task by ID |
| PUT | `/api/tasks/{id}` | Update task |
| DELETE | `/api/tasks/{id}` | Delete task |

### Users

| Method | Path | Description |
|--------|------|-------------|
| POST | `/api/login` | Login |
| POST | `/api/logout` | Logout |
| GET | `/api/me` | Current user info |

### WebSocket

Connect to `/ws` for real-time updates:

```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    // data.type: 'task_created', 'task_updated', 'task_deleted'
    // data.payload: task object
};
```

## Running Tests

```bash
cmake --build build --target project_tests
./build/v2/examples/Project/project_tests
```

## Documentation

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for production guidelines.
