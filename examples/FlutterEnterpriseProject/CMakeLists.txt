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
target_compile_features(task_dashboard PRIVATE cxx_std_20)

# Flutter Shared Library (FFI Bridge)
set(FFI_SOURCES ${PROJECT_SOURCES})
list(REMOVE_ITEM FFI_SOURCES src/main.cpp)
list(APPEND FFI_SOURCES src/ffi_bridge.cpp)

add_library(task_dashboard_ffi SHARED ${FFI_SOURCES})
target_link_libraries(task_dashboard_ffi PRIVATE coroute)
target_include_directories(task_dashboard_ffi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(task_dashboard_ffi PRIVATE cxx_std_20)

# Set output properties so coroute_framework Dart code can load it
set_target_properties(task_dashboard_ffi PROPERTIES 
    OUTPUT_NAME "coroute_app"
)
if(NOT MSVC)
    set_target_properties(task_dashboard_ffi PROPERTIES PREFIX "lib")
endif()

target_compile_definitions(task_dashboard_ffi PRIVATE COROUTE_CLIENT_MODE)

# =========================================================================
# Flutter Project Configuration
# =========================================================================
set(FLUTTER_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.flutter")
set(USER_PUBSPEC_IN "${CMAKE_CURRENT_SOURCE_DIR}/pubspec.yaml.in")

if(EXISTS "${USER_PUBSPEC_IN}")
    file(RELATIVE_PATH COROUTE_FRAMEWORK_REL_PATH 
        "${FLUTTER_PROJECT_DIR}" 
        "${CMAKE_SOURCE_DIR}/packages/coroute_framework"
    )
    file(READ "${USER_PUBSPEC_IN}" PUBSPEC_CONTENT)
    string(REPLACE "%%COROUTE_FRAMEWORK_REL_PATH%%" "${COROUTE_FRAMEWORK_REL_PATH}" PUBSPEC_CONTENT "${PUBSPEC_CONTENT}")
    file(MAKE_DIRECTORY "${FLUTTER_PROJECT_DIR}")
    file(WRITE "${FLUTTER_PROJECT_DIR}/pubspec.yaml" "${PUBSPEC_CONTENT}")
endif()

file(MAKE_DIRECTORY "${FLUTTER_PROJECT_DIR}/lib")

if(NOT EXISTS "${FLUTTER_PROJECT_DIR}/macos")
    message(STATUS "Initializing Flutter platform runners for FlutterEnterpriseProject...")
    execute_process(
        COMMAND flutter create . --platforms=android,ios,macos,web,windows,linux --suppress-analytics
        WORKING_DIRECTORY "${FLUTTER_PROJECT_DIR}"
    )
endif()

function(ensure_linked SOURCE DEST)
    if(EXISTS "${SOURCE}" AND NOT EXISTS "${DEST}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${SOURCE}" "${DEST}")
    endif()
endfunction()

if(EXISTS "${FLUTTER_PROJECT_DIR}/lib/main.dart" AND NOT IS_SYMLINK "${FLUTTER_PROJECT_DIR}/lib/main.dart")
     file(REMOVE "${FLUTTER_PROJECT_DIR}/lib/main.dart")
endif()

ensure_linked("${CMAKE_CURRENT_SOURCE_DIR}/main.dart" "${FLUTTER_PROJECT_DIR}/lib/main.dart")
ensure_linked("${CMAKE_CURRENT_SOURCE_DIR}/templates" "${FLUTTER_PROJECT_DIR}/lib/templates")

add_custom_command(TARGET task_dashboard_ffi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:task_dashboard_ffi> ${FLUTTER_PROJECT_DIR}/
    COMMENT "Copying shared library to Source Root for Flutter Run"
)

add_custom_target(run_flutter_mobile
    COMMAND flutter run
    WORKING_DIRECTORY ${FLUTTER_PROJECT_DIR}
    DEPENDS task_dashboard_ffi
    USES_TERMINAL
)

add_custom_target(run_flutter_desktop
    COMMAND flutter run -d windows
    WORKING_DIRECTORY ${FLUTTER_PROJECT_DIR}
    DEPENDS task_dashboard_ffi
    USES_TERMINAL
)


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
