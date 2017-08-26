cmake_minimum_required(VERSION 3.7)
project(runas)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)


set(SOURCE_FILES
        include/conf.h
        include/perm.h
        include/utils.h
        include/path.h
        include/version.h
        src/conf.cpp
        src/perm.cpp
        src/runas.cpp
        src/utils.cpp
        src/path.cpp)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_INSTALL_PREFIX /usr/local)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-O0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")


add_executable(runas ${SOURCE_FILES})

install (TARGETS runas
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_EXECUTE SETUID
        RUNTIME DESTINATION bin)


