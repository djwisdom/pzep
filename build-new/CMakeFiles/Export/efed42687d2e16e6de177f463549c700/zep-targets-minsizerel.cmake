#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Zep::Zep" for configuration "MinSizeRel"
set_property(TARGET Zep::Zep APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(Zep::Zep PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/Zep.lib"
  )

list(APPEND _cmake_import_check_targets Zep::Zep )
list(APPEND _cmake_import_check_files_for_Zep::Zep "${_IMPORT_PREFIX}/lib/Zep.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
