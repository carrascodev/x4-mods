# CMake utilities for X4 mods

# Function to generate Lua wrappers for a mod
macro(generate_lua_wrappers MOD_NAME)
    # When called from a mod's CMakeLists.txt, CMAKE_CURRENT_SOURCE_DIR is the mod directory
    set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/generated")
    set(HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src/public")
    set(COMMON_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/cpp/src/public")
    # Path to the script - hardcoded for now
    set(SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../HenMod.Commons/scripts/generate_lua_wrappers.py")

    # Force regeneration if REGENERATE_LUA_WRAPPERS is set
    if(REGENERATE_LUA_WRAPPERS OR NOT EXISTS "${GENERATED_DIR}/generated_files.cmake")
        execute_process(
            COMMAND python ${SCRIPT_PATH}
                ${HEADER_DIR}
                ${COMMON_HEADER_DIR}
                --output_dir ${GENERATED_DIR}
        )
    endif()

    if(EXISTS "${GENERATED_DIR}/generated_files.cmake")
        include(${GENERATED_DIR}/generated_files.cmake)
    endif()

    # Add custom target for manual regeneration
    add_custom_target(generate_wrappers_${MOD_NAME}
        COMMAND python ${SCRIPT_PATH}
            ${HEADER_DIR}
            ${COMMON_HEADER_DIR}
            --output_dir ${GENERATED_DIR}
        COMMENT "Generating Lua wrappers for ${MOD_NAME}"
    )
endmacro()