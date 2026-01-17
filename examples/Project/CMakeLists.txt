# Task Dashboard - Production Example Project
cmake_minimum_required(VERSION 3.20)

# Collect all source files
set(PROJECT_SOURCES
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
)

# Main executable
add_executable(task_dashboard ${PROJECT_SOURCES})
target_link_libraries(task_dashboard PRIVATE coroute)
target_include_directories(task_dashboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable C++20
target_compile_features(task_dashboard PRIVATE cxx_std_20)

# Tests
if(COROUTE_BUILD_TESTS)
    # set(TEST_SOURCES
    #     tests/unit/models/test_task.cpp
    #     tests/unit/models/test_user.cpp
    #     tests/unit/services/test_task_service.cpp
    #     tests/unit/services/test_user_service.cpp
    #     tests/unit/middleware/test_auth.cpp
    #     tests/integration/test_api_tasks.cpp
    #     tests/integration/test_api_users.cpp
    #     tests/integration/test_websocket.cpp
    # )
    # 
    # add_executable(project_tests ${TEST_SOURCES})
    # target_link_libraries(project_tests PRIVATE coroute Catch2::Catch2WithMain)
    # target_include_directories(project_tests PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/src
    #     ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
    # )
    # 
    # # Register tests with CTest
    # add_test(NAME ProjectTests COMMAND project_tests)
endif()
