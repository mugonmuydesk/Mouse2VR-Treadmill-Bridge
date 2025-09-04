#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ViGEmClient::ViGEmClient" for configuration "RelWithDebInfo"
set_property(TARGET ViGEmClient::ViGEmClient APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(ViGEmClient::ViGEmClient PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX;RC"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/ViGEmClient.lib"
  )

list(APPEND _cmake_import_check_targets ViGEmClient::ViGEmClient )
list(APPEND _cmake_import_check_files_for_ViGEmClient::ViGEmClient "${_IMPORT_PREFIX}/lib/ViGEmClient.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
