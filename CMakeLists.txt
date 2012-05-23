PROJECT(KPackage)

# LIST OF HEADERS FILES
SET(HEADER_FILES 
        include/VFS.h
        include/VFS_Config.h
        include/VFS_Implementation.h)
        
# LIST OF SOURCES FILES
SET(SOURCE_FILES
        src/VFS_Archive.cpp
        src/VFS_ArchiveFile.cpp
        src/VFS_Archives.cpp
        src/VFS_Basic.cpp
        src/VFS_Dirs.cpp
        src/VFS_StdIOFile.cpp
        src/VFS_Utilities.cpp)

include_directories(include/)

add_definitions(-DUNIX -DLINUX -DUSE_STL)
add_library(KPackage STATIC ${SOURCE_FILES})
		
