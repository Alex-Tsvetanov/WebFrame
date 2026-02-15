function(add_coroute_app TARGET_NAME)
    # Parse arguments
    cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDENCIES;PLATFORMS" ${ARGN})

    message(STATUS "Configuring Coroute App: ${TARGET_NAME}")
    message(STATUS "  - Platforms: ${ARG_PLATFORMS}")
    foreach(PLATFORM ${ARG_PLATFORMS})
        message(STATUS "    - Platform: ${PLATFORM}")
    endforeach()

    # =========================================================================
    # 1. C++ Logic (Shared Library)
    # =========================================================================
    
    # Define the shared library target for the application logic
    # This will be loaded by the Flutter app via FFI
    add_library(${TARGET_NAME}_shared SHARED
        ${ARG_SOURCES}
        # Include bridge automatically if not present? 
        # For now assume user includes/links what they need, OR we add bridge here
        ${CMAKE_SOURCE_DIR}/src/bridge/bridge.cpp
    )

    # Compile definitions
    target_compile_definitions(${TARGET_NAME}_shared PRIVATE 
        main=app_main
        COROUTE_HAS_TEMPLATES
        COROUTE_CLIENT_MODE # Default for mobile/desktop apps
    )

    # Link dependencies
    target_link_libraries(${TARGET_NAME}_shared PRIVATE 
        coroute 
        ${ARG_DEPENDENCIES}
    )
    
    # Platform-specific linking options (Whole Archive)
    if(APPLE)
        target_link_libraries(${TARGET_NAME}_shared PRIVATE -Wl,-force_load coroute)
    elseif(MSVC)
        target_link_libraries(${TARGET_NAME}_shared PRIVATE /WHOLEARCHIVE:coroute)
    else()
        target_link_libraries(${TARGET_NAME}_shared PRIVATE -Wl,--whole-archive coroute -Wl,--no-whole-archive)
    endif()

    # Include directories
    target_include_directories(${TARGET_NAME}_shared PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
    )

    # Output name (important for FFI loading)
    set_target_properties(${TARGET_NAME}_shared PROPERTIES 
        OUTPUT_NAME "coroute_app"
        PREFIX "lib" # Force lib prefix even on Windows for consistency/simplicity if needed, or handle in Dart
    )


    # =========================================================================
    # 2. Flutter Project Generation
    # =========================================================================

    # =========================================================================
    # 2. Flutter Project Configuration (In-Source Subdirectory)
    # =========================================================================

    # We configure the Flutter project IN A SUBDIRECTORY of the source
    # This keeps the source root clean while allowing fast access.
    # Directory: .flutter/ (Hidden)

    set(FLUTTER_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.flutter")
    set(USER_PUBSPEC_IN "${CMAKE_CURRENT_SOURCE_DIR}/pubspec.yaml.in")
    
    # 2a. Configure pubspec.yaml
    if(EXISTS "${USER_PUBSPEC_IN}")
        # Calculate relative path to coroute_framework from FLUTTER_PROJECT_DIR
        file(RELATIVE_PATH COROUTE_FRAMEWORK_REL_PATH 
            "${FLUTTER_PROJECT_DIR}" 
            "${CMAKE_SOURCE_DIR}/packages/coroute_framework"
        )
        
        # Configure the file
        file(READ "${USER_PUBSPEC_IN}" PUBSPEC_CONTENT)
        string(REPLACE "%%COROUTE_FRAMEWORK_REL_PATH%%" "${COROUTE_FRAMEWORK_REL_PATH}" PUBSPEC_CONTENT "${PUBSPEC_CONTENT}")
        
        # Ensure dir exists
        file(MAKE_DIRECTORY "${FLUTTER_PROJECT_DIR}")
        file(WRITE "${FLUTTER_PROJECT_DIR}/pubspec.yaml" "${PUBSPEC_CONTENT}")
    endif()

    # 2b. setup lib/ structure with Symlinks
    # Flutter requires code in lib/. User has code at root.
    # We create lib/ if missing and symlink main.dart and templates.
    
    file(MAKE_DIRECTORY "${FLUTTER_PROJECT_DIR}/lib")

    # Initialize Flutter Project if needed (generates macos/android/ios runners)
    if(NOT EXISTS "${FLUTTER_PROJECT_DIR}/macos")
        message(STATUS "Initializing Flutter platform runners in ${FLUTTER_PROJECT_DIR}...")
        # We use execute_process directly here to ensure it happens during configuration
        # checking the command result
        execute_process(
            COMMAND flutter create . --platforms=android,ios,macos,web,windows,linux --suppress-analytics
            WORKING_DIRECTORY "${FLUTTER_PROJECT_DIR}"
            RESULT_VARIABLE FLUTTER_CREATE_RESULT
        )
        if(NOT FLUTTER_CREATE_RESULT EQUAL 0)
            message(WARNING "Failed to initialize Flutter project. 'flutter create' failed.")
        endif()
    endif()
    
    # Symlink/Copy Helper
    function(ensure_linked SOURCE DEST)
        if(EXISTS "${SOURCE}")
            # Identify if it's a directory
            if(IS_DIRECTORY "${SOURCE}")
                # For directories, on Windows we might need junction or copy. 
                # On Unix symlink is fine.
                # cmake -E create_symlink handles files. For dirs?
                # Let's assume copy for robustness if symlink fails, but try symlink?
                # Adding symlinks to source dir is risky if not careful.
                # Assuming user wants this structure.
                
                # Check if dest exists
                if(NOT EXISTS "${DEST}")
                     # Create symlink
                     execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${SOURCE}" "${DEST}")
                endif()
            else()
                # File
                if(NOT EXISTS "${DEST}")
                    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${SOURCE}" "${DEST}")
                endif()
            endif()
        endif()
    endfunction()

    # Remove default main.dart if it exists (so we can symlink the user's one)
    # But only if it's not already a symlink (avoid churn? or just force it)
    # Simple approach: remove. verify later.
    # Note: IS_SYMLINK check in CMake is tricky cross-platform.
    # We'll just remove it if we just created the project or always?
    # Safer to always ensure it's linked correctly.
    # checking if it matches creation?
    
    # We will FORCE the link for main.dart
    if(EXISTS "${FLUTTER_PROJECT_DIR}/lib/main.dart" AND NOT IS_SYMLINK "${FLUTTER_PROJECT_DIR}/lib/main.dart")
         file(REMOVE "${FLUTTER_PROJECT_DIR}/lib/main.dart")
    endif()

    ensure_linked("${CMAKE_CURRENT_SOURCE_DIR}/main.dart" "${FLUTTER_PROJECT_DIR}/lib/main.dart")
    ensure_linked("${CMAKE_CURRENT_SOURCE_DIR}/templates" "${FLUTTER_PROJECT_DIR}/lib/templates")
    ensure_linked("${CMAKE_CURRENT_SOURCE_DIR}/viewmodels" "${FLUTTER_PROJECT_DIR}/lib/viewmodels")

    # 2c. Copy Shared Library to Source Root (for FFI)
    add_custom_command(TARGET ${TARGET_NAME}_shared POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_NAME}_shared> ${FLUTTER_PROJECT_DIR}/
        COMMENT "Copying shared library to Source Root for Flutter Run"
    )

    # =========================================================================
    # 3. Run Targets
    # =========================================================================
    
    if("Mobile" IN_LIST ARG_PLATFORMS)
        add_custom_target(${TARGET_NAME}_mobile
            COMMAND flutter run
            WORKING_DIRECTORY ${FLUTTER_PROJECT_DIR}
            DEPENDS ${TARGET_NAME}_shared
            USES_TERMINAL
        )
    endif()
    
    if("Desktop" IN_LIST ARG_PLATFORMS)
        add_custom_target(${TARGET_NAME}_desktop
            COMMAND flutter run -d macos
            WORKING_DIRECTORY ${FLUTTER_PROJECT_DIR}
            DEPENDS ${TARGET_NAME}_shared
            USES_TERMINAL
        )
    endif()

    if("Web" IN_LIST ARG_PLATFORMS)
        message(STATUS "Defining Native Web Server Target: ${TARGET_NAME}_server")
        # Web means "Native Web Server" in this context
        add_executable(${TARGET_NAME}_server ${ARG_SOURCES})
        
        target_link_libraries(${TARGET_NAME}_server PRIVATE 
            coroute 
            ${ARG_DEPENDENCIES}
        )

        # Platform-specific linking options (Whole Archive) - Required for coroute static/factory registration
        if(APPLE)
            target_link_libraries(${TARGET_NAME}_server PRIVATE -Wl,-force_load coroute)
        elseif(MSVC)
            target_link_libraries(${TARGET_NAME}_server PRIVATE /WHOLEARCHIVE:coroute)
        else()
            target_link_libraries(${TARGET_NAME}_server PRIVATE -Wl,--whole-archive coroute -Wl,--no-whole-archive)
        endif()
        
        target_compile_definitions(${TARGET_NAME}_server PRIVATE 
            COROUTE_HAS_TEMPLATES
            # COROUTE_CLIENT_MODE is NOT defined, so it runs in server mode
        )
        
        target_include_directories(${TARGET_NAME}_server PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/include
        )

        add_custom_target(${TARGET_NAME}_web
            COMMAND $<TARGET_FILE:${TARGET_NAME}_server>
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${TARGET_NAME}_server
            USES_TERMINAL
        )
    endif()

endfunction()
