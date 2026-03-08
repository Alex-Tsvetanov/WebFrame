add_coroute_app(flutter_task_dashboard
    SOURCES
        src/main.cpp
        src/app/config.cpp
        src/app/server.cpp
        src/middleware/auth.cpp
        src/middleware/request_logger.cpp
        src/handlers/pages.cpp
        src/handlers/api/tasks.cpp
        src/handlers/api/users.cpp
        src/handlers/websocket/task_hub.cpp
        src/models/task.cpp
        src/models/user.cpp
        src/services/task_service.cpp
        src/services/user_service.cpp
    PLATFORMS Mobile Desktop Web
)

# All headers use paths relative to src/, e.g. "middleware/auth.hpp"
target_include_directories(flutter_task_dashboard_shared PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(flutter_task_dashboard_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
