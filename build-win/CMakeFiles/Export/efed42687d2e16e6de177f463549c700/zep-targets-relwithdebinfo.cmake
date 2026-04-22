#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Zep::Zep" for configuration "RelWithDebInfo"
set_property(TARGET Zep::Zep APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Zep::Zep PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/Zep-reldbg.lib"
  )

list(APPEND _cmake_import_check_targets Zep::Zep )
list(APPEND _cmake_import_check_files_for_Zep::Zep "${_IMPORT_PREFIX}/lib/Zep-reldbg.lib" )

# Import target "Zep::unittests" for configuration "RelWithDebInfo"
set_property(TARGET Zep::unittests APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Zep::unittests PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/unittests.exe"
  )

list(APPEND _cmake_import_check_targets Zep::unittests )
list(APPEND _cmake_import_check_files_for_Zep::unittests "${_IMPORT_PREFIX}/bin/unittests.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
