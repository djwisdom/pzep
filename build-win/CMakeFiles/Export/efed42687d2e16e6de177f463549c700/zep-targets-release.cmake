#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Zep::Zep" for configuration "Release"
set_property(TARGET Zep::Zep APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Zep::Zep PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/Zep.lib"
  )

list(APPEND _cmake_import_check_targets Zep::Zep )
list(APPEND _cmake_import_check_files_for_Zep::Zep "${_IMPORT_PREFIX}/lib/Zep.lib" )

# Import target "Zep::unittests" for configuration "Release"
set_property(TARGET Zep::unittests APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Zep::unittests PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/unittests.exe"
  )

list(APPEND _cmake_import_check_targets Zep::unittests )
list(APPEND _cmake_import_check_files_for_Zep::unittests "${_IMPORT_PREFIX}/bin/unittests.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
