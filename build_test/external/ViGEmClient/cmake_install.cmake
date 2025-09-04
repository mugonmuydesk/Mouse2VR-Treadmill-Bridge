# Install script for directory: C:/Dev/Mouse2VR-Treadmill-Bridge/external/ViGEmClient

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/Mouse2VR")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/lib/Debug/ViGEmClient.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/lib/Release/ViGEmClient.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/lib/MinSizeRel/ViGEmClient.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/lib/RelWithDebInfo/ViGEmClient.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ViGEm" TYPE FILE FILES
    "C:/Dev/Mouse2VR-Treadmill-Bridge/external/ViGEmClient/include/ViGEm/km/BusShared.h"
    "C:/Dev/Mouse2VR-Treadmill-Bridge/external/ViGEmClient/include/ViGEm/Client.h"
    "C:/Dev/Mouse2VR-Treadmill-Bridge/external/ViGEmClient/include/ViGEm/Common.h"
    "C:/Dev/Mouse2VR-Treadmill-Bridge/external/ViGEmClient/include/ViGEm/Util.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient" TYPE FILE FILES
    "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/ViGEmClientConfig.cmake"
    "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/ViGEmClientConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient/ViGEmClientTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient/ViGEmClientTargets.cmake"
         "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/CMakeFiles/Export/9b1e94ee417f6f368505c1e5c9ad31b4/ViGEmClientTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient/ViGEmClientTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient/ViGEmClientTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient" TYPE FILE FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/CMakeFiles/Export/9b1e94ee417f6f368505c1e5c9ad31b4/ViGEmClientTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient" TYPE FILE FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/CMakeFiles/Export/9b1e94ee417f6f368505c1e5c9ad31b4/ViGEmClientTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient" TYPE FILE FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/CMakeFiles/Export/9b1e94ee417f6f368505c1e5c9ad31b4/ViGEmClientTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient" TYPE FILE FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/CMakeFiles/Export/9b1e94ee417f6f368505c1e5c9ad31b4/ViGEmClientTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ViGEmClient" TYPE FILE FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/CMakeFiles/Export/9b1e94ee417f6f368505c1e5c9ad31b4/ViGEmClientTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/ViGEmClient.pc")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/dev/Mouse2VR-Treadmill-Bridge/build_test/external/ViGEmClient/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
