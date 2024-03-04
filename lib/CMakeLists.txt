cmake_minimum_required (VERSION 3.14)

# Sockets for windows should be linked
if (WIN32)
    list(APPEND EXTRA_LIBS wsock32 ws2_32)
    add_definitions(-DNOMINMAX)
endif()

# Multi-threading should be available
find_package(Threads REQUIRED)
list(APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})

## Add necessary flags
if(CMAKE_USE_PTHREADS_INIT) 
    add_definitions(-DUSE_PTHREAD)
endif()

# URL Matcher should be compiled
add_subdirectory("url-matcher")
set_target_properties(regexmatcher_core PROPERTIES COMPILE_FLAGS ${DEPENDANCES_WARNING_LEVEL})
list(APPEND EXTRA_LIBS RegexMatcher::core)

# Add inja if required
if(USE_INJA)
    set(INJA_BUILD_TESTS OFF CACHE INTERNAL "disables the inja tests" FORCE)
    add_subdirectory("inja")
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inja" APPEND PROPERTY VARIABLE BUILD_TESTING OFF)
    set_target_properties(inja PROPERTIES COMPILE_FLAGS ${DEPENDANCES_WARNING_LEVEL})
    add_definitions(-DUSE_INJA)
    list(APPEND EXTRA_LIBS pantor::inja)
endif()

# Compile Webframe::Core
add_library(Core STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/core/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/http/processing/request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/http/request/method.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/http/response/response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/http/response/status_line.cpp
)
target_link_libraries(Core PUBLIC ${EXTRA_LIBS})
target_include_directories(Core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/")
set_target_properties(Core PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS Core DESTINATION ${PROJECT_BINARY_DIR})
