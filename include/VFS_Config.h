//****************************************************************************
//**
//**    VFS_TYPES.H
//**    Header - Virtual File System Configuration
//**
//**    Project:        KPackage
//**    Component:      KPackage
//**    Author:         Michael Walter
//**                    Keinier Caboverde <Keinier@gmail.com>
//**
//**    History:
//**            25.07.2001              Created (Michael Walter)
//**            22.05.2012              Unix Ported(Keinier Caboverde)
//**
//****************************************************************************
#ifndef VFS_VFS_CONFIG_H
#define VFS_VFS_CONFIG_H

//============================================================================
//    COMPILER SETTINGS
//============================================================================
#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

//============================================================================
//    INTERFACE REQUIRED HEADERS
//============================================================================
#include <vector>
#include <map>
#include <string>
#include <cstring>

//============================================================================
//    WIN32 CONFIGURATION
//============================================================================
#ifdef _WIN32

#	include <windows.h>
#	include <io.h>
#	if defined( _MSC_VER )
#		pragma comment( lib, "vfs.lib" )
#		pragma warning( disable : 4311 )
#		pragma warning( disable : 4312 )
#		if defined( _DEBUG )
#			define VFS_DEBUG
#		endif
#	endif

//============================================================================
//    PLATFORM-DEPENDANT FUNCTIONS
//============================================================================
#	define VFS_UNLINK( strAbsoluteFileName )			( ( _wunlink( ( strAbsoluteFileName ).c_str() ) == 0 ) ? VFS_TRUE : VFS_FALSE )
#	define VFS_RENAME( strAbsoluteFileName, strTo )		( ( _wrename( ( strAbsoluteFileName ).c_str(), strTo.c_str() ) == 0 ) ? VFS_TRUE : VFS_FALSE )
#	define VFS_MKDIR( strAbsoluteDirName )				( ( _wmkdir( ( strAbsoluteDirName ).c_str() ) == 0 ) ? VFS_TRUE : VFS_FALSE )
#	define VFS_RMDIR( strAbsoluteDirName )				( ( _wrmdir( ( strAbsoluteDirName ).c_str() ) == 0 ) ? VFS_TRUE : VFS_FALSE )
#	define VFS_EXISTS( strAbsoluteFileName )			( ( GetFileAttributesW( ( strAbsoluteFileName ).c_str() ) != 0xFFFFFFFF ) ? VFS_TRUE : VFS_FALSE )
#	define VFS_IS_DIR( strAbsoluteDirName )				( ( GetFileAttributesW( ( strAbsoluteDirName ).c_str() ) != 0xFFFFFFFF && ( GetFileAttributesW( strAbsoluteDirName.c_str() ) & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY ) ? VFS_TRUE : VFS_FALSE )
#	define VFS_FOPEN( strAbsoluteFileName, strAccess )	( _wfopen( ( strAbsoluteFileName ).c_str(), ( strAccess ).c_str() ) )
#	define VFS_RESIZE( pFile, lSize )					( ( _chsize( m_pFile->_file, lSize ) == 0 ) ? VFS_TRUE : VFS_FALSE )
#	define VFS_GETSIZE( pFile )							( _filelength( m_pFile->_file ) )

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
// Numeric Types.
typedef bool VFS_BOOL;
typedef unsigned char VFS_BYTE;
typedef unsigned short VFS_WORD;
typedef unsigned long VFS_DWORD;
typedef int VFS_INT;
typedef unsigned int VFS_UINT;
typedef long VFS_LONG;

// Numeric Macros.
static const VFS_BOOL VFS_TRUE = true;
static const VFS_BOOL VFS_FALSE = false;

// String types.
typedef wchar_t VFS_CHAR;
typedef wchar_t *VFS_PSTR;
typedef const wchar_t *VFS_PCSTR;
typedef std::wstring VFS_String;

// String Macros.
#define VFS_TEXT( string )			L ## string

// Loword/Hiword Macros (for Versioning)
#define VFS_HIBYTE( word )		( ( VFS_BYTE )( ( ( VFS_WORD ) word ) >> 8 ) )
#define VFS_LOBYTE( word )		( ( VFS_BYTE )( ( ( VFS_WORD ) word ) & 0xFF ) )
#define VFS_HIWORD( dword )		( ( VFS_WORD )( ( ( VFS_DWORD ) dword ) >> 16 ) )
#define VFS_LOWORD( dword )		( ( VFS_WORD )( ( ( VFS_DWORD ) dword ) & 0xFFFF ) )
#define VFS_MAKE_WORD( lo, hi )	( ( VFS_WORD )( ( ( VFS_BYTE ) lo ) | ( ( VFS_WORD ) ( ( ( VFS_BYTE ) hi ) << 8 ) ) ) )
#define VFS_MAKE_DWORD( lo, hi )	( ( VFS_DWORD )( ( ( VFS_WORD ) lo ) | ( ( VFS_DWORD ) ( ( ( VFS_WORD ) hi ) << 16 ) ) ) )

//============================================================================
//    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    INTERFACE STRUCTURES / UTILITY CLASSES
//============================================================================
//============================================================================
//    INTERFACE DATA DECLARATIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================
inline VFS_BOOL VFS_FIND_FILE(const VFS_String & strAbsoluteFileName, VFS_String & strFoundName, VFS_BOOL & bIsDir, VFS_LONG & lSize, VFS_INT nMode)
{
    static HANDLE hFindFile = NULL;
    WIN32_FIND_DATAW wfd;
    if (nMode == 0) {
	if ((hFindFile = FindFirstFileW(strAbsoluteFileName.c_str(), &wfd)) == INVALID_HANDLE_VALUE)
	    return VFS_FALSE;
	strFoundName = wfd.cFileName;
	bIsDir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	lSize = wfd.nFileSizeLow;
	return VFS_TRUE;
    } else if (nMode == 1) {
	if (!FindNextFileW(hFindFile, &wfd))
	    return VFS_FALSE;
	strFoundName = wfd.cFileName;
	bIsDir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	lSize = wfd.nFileSizeLow;
	return VFS_TRUE;
    } else if (nMode == 2) {
	return FindClose(hFindFile) ? VFS_TRUE : VFS_FALSE;
    }
    return VFS_FALSE;
}

//============================================================================
//    INTERFACE OBJECT CLASS DEFINITIONS
//============================================================================
//============================================================================
//    TRAILING CONFIGURATION SECTIONS
//============================================================================
//============================================================================
//    INTERFACE TRAILING HEADERS
//============================================================================

#else
// configuration standar
#	include <stdio.h>
#	include <stdlib.h>
#       include <dirent.h>
#       include <errno.h>
#       include <sys/stat.h>
#       include <sys/types.h>
#       include <unistd.h>
#	if defined( _MSC_VER )
#		if defined( _DEBUG ) || defined (DEBUG)
#			define VFS_DEBUG
#		endif
#	endif
typedef std::string VFS_String;
//============================================================================
//    PLATFORM-DEPENDANT FUNCTIONS
//============================================================================
#define VFS_UNLINK( strAbsoluteFileName ) (( remove((strAbsoluteFileName).c_str()) == 0) ? VFS_TRUE : VFS_FALSE )

#define VFS_RENAME( strAbsoluteFileName, strTo ) (( rename((strAbsoluteFileName).c_str(),strTo.c_str())==0) ? VFS_TRUE : VFS_FALSE )

#define VFS_MKDIR( strAbsoluteDirName )	( ( mkdir( ( strAbsoluteDirName ).c_str(),0777 ) == 0 ) ? VFS_TRUE : VFS_FALSE )

#define VFS_RMDIR( strAbsoluteDirName )	( ( rmdir( ( strAbsoluteDirName ).c_str() ) == 0 ) ? VFS_TRUE : VFS_FALSE )
static struct stat stat_buffer;
#define VFS_EXISTS( strAbsoluteFileName ) ( ( stat( ( strAbsoluteFileName ).c_str(),&stat_buffer ) == 0 ) ? VFS_TRUE : VFS_FALSE )

bool isdir(VFS_String target)
{
        struct stat buff;
        if(!stat(target.c_str(),&buff))
        {
                if(buff.st_mode & S_IFDIR)
                        return true;
                        
        }
        return false;
}
long long getsize(FILE* target)
{  
                fseek(target,0,SEEK_END);
                return ftell(target);
}

#define VFS_IS_DIR( strAbsoluteDirName ) ( ( isdir((strAbsoluteDirName))) ? VFS_TRUE : VFS_FALSE )

#define VFS_FOPEN( strAbsoluteFileName, strAccess ) ( fopen( ( strAbsoluteFileName ).c_str(), ( strAccess ).c_str() ) )

#define VFS_RESIZE( pFile, lSize ) ( ( chsize( m_pFile->_file, lSize ) == 0 ) ? VFS_TRUE : VFS_FALSE )

#define VFS_GETSIZE( pFile ) ( getsize( pFile) )

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
// Numeric Types.
typedef bool VFS_BOOL;
typedef unsigned char VFS_BYTE;
typedef unsigned short VFS_WORD;
typedef unsigned long VFS_DWORD;
typedef int VFS_INT;
typedef unsigned int VFS_UINT;
typedef long VFS_LONG;

// Numeric Macros.
static const VFS_BOOL VFS_TRUE = true;
static const VFS_BOOL VFS_FALSE = false;

// String types.
typedef char VFS_CHAR;
typedef char *VFS_PSTR;
typedef const char *VFS_PCSTR;
typedef void* HANDLE;

// String Macros.
#define VFS_TEXT( string )			string

// Loword/Hiword Macros (for Versioning)
#define VFS_HIBYTE( word )		( ( VFS_BYTE )( ( ( VFS_WORD ) word ) >> 8 ) )
#define VFS_LOBYTE( word )		( ( VFS_BYTE )( ( ( VFS_WORD ) word ) & 0xFF ) )
#define VFS_HIWORD( dword )		( ( VFS_WORD )( ( ( VFS_DWORD ) dword ) >> 16 ) )
#define VFS_LOWORD( dword )		( ( VFS_WORD )( ( ( VFS_DWORD ) dword ) & 0xFFFF ) )
#define VFS_MAKE_WORD( lo, hi )	( ( VFS_WORD )( ( ( VFS_BYTE ) lo ) | ( ( VFS_WORD ) ( ( ( VFS_BYTE ) hi ) << 8 ) ) ) )
#define VFS_MAKE_DWORD( lo, hi )	( ( VFS_DWORD )( ( ( VFS_WORD ) lo ) | ( ( VFS_DWORD ) ( ( ( VFS_WORD ) hi ) << 16 ) ) ) )

//============================================================================
//    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    INTERFACE STRUCTURES / UTILITY CLASSES
//============================================================================
//============================================================================
//    INTERFACE DATA DECLARATIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================
inline VFS_BOOL VFS_FIND_FILE(const VFS_String & strAbsoluteFileName, VFS_String & strFoundName, VFS_BOOL & bIsDir, VFS_LONG & lSize, VFS_INT nMode)
{
    /*static HANDLE hFindFile = NULL;
    WIN32_FIND_DATAW wfd;
    if (nMode == 0) {
	if ((hFindFile = FindFirstFileW(strAbsoluteFileName.c_str(), &wfd)) == INVALID_HANDLE_VALUE)
	    return VFS_FALSE;
	strFoundName = wfd.cFileName;
	bIsDir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	lSize = wfd.nFileSizeLow;
	return VFS_TRUE;
    } else if (nMode == 1) {
	if (!FindNextFileW(hFindFile, &wfd))
	    return VFS_FALSE;
	strFoundName = wfd.cFileName;
	bIsDir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	lSize = wfd.nFileSizeLow;
	return VFS_TRUE;
    } else if (nMode == 2) {
	return FindClose(hFindFile) ? VFS_TRUE : VFS_FALSE;
    }*/
    return VFS_FALSE;
}

//============================================================================
//    INTERFACE OBJECT CLASS DEFINITIONS
//============================================================================
//============================================================================
//    TRAILING CONFIGURATION SECTIONS
//============================================================================
//============================================================================
//    INTERFACE TRAILING HEADERS
//============================================================================

#endif

#endif				// __VFS_VFS_TYPES_H__

