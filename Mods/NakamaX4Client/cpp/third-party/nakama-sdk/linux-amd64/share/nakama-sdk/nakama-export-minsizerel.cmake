#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nakama-sdk" for configuration "MinSizeRel"
set_property(TARGET nakama-sdk APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(nakama-sdk PROPERTIES
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/libnakama-sdk.so"
  IMPORTED_SONAME_MINSIZEREL "libnakama-sdk.so"
  )

list(APPEND _cmake_import_check_targets nakama-sdk )
list(APPEND _cmake_import_check_files_for_nakama-sdk "${_IMPORT_PREFIX}/lib/libnakama-sdk.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
