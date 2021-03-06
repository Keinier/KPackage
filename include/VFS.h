#ifndef VFS_VFS_H
#define VFS_VFS_H

//Headers
#include "VFS_Config.h"

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
// The Handle Type.
enum VFS_Handle { VFS_HANDLE_FORCE_DWORD = 0xFFFFFFFF };

// Various Constants.
static const VFS_WORD VFS_VERSION = VFS_MAKE_WORD(0, 1);	// Version 1.0
static const VFS_CHAR VFS_PATH_SEPARATOR = VFS_TEXT('/');
static const VFS_CHAR *VFS_ARCHIVE_FILE_EXTENSION = VFS_TEXT("DAGN");
static const int VFS_MAX_NAME_LENGTH = 64;
static const VFS_Handle VFS_INVALID_HANDLE_VALUE = (VFS_Handle) 0;
static const VFS_DWORD VFS_INVALID_DWORD_VALUE = 0xFFFFFFFF;
static const VFS_LONG VFS_INVALID_LONG_VALUE = -1;
#define						VFS_INVALID_POINTER_VALUE	NULL

// The File_Open/Create() Flags.
enum VFS_OpenFlags {
    VFS_READ = 0x0001,
    VFS_WRITE = 0x0002
};

// The File_Seek() Origin.
enum VFS_SeekOrigin {
    VFS_SET,
    VFS_CURRENT,
    VFS_END
};

// The Error Constants.
enum VFS_ErrorCode {
    VFS_ERROR_NONE,
    VFS_ERROR_NOT_INITIALIZED_YET,
    VFS_ERROR_ALREADY_INITIALIZED,
    VFS_ERROR_ALREADY_EXISTS,
    VFS_ERROR_NOT_FOUND,
    VFS_ERROR_INVALID_PARAMETER,
    VFS_ERROR_GENERIC,
    VFS_ERROR_INVALID_ERROR_CODE,
    VFS_ERROR_NO_ROOT_PATHS_DEFINED,
    VFS_ERROR_PERMISSION_DENIED,
    VFS_ERROR_IN_USE,
    VFS_ERROR_CANT_MANIPULATE_ARCHIVES,
    VFS_ERROR_NOT_AN_ARCHIVE,
    VFS_ERROR_INVALID_ARCHIVE_FORMAT,
    VFS_ERROR_MISSING_FILTERS,
    VFS_NUM_ERRORS
};

// The Type of an Entity
enum VFS_EntityType {
    VFS_FILE,
    VFS_DIR,
    VFS_ARCHIVE
};

// The Filter Reader and Writer Procedures.
typedef VFS_BOOL(*VFS_FilterReadProc) (VFS_BYTE * pBuffer, VFS_DWORD dwBytesToRead, VFS_DWORD * pBytesRead);
typedef VFS_BOOL(*VFS_FilterWriteProc) (const VFS_BYTE * pBuffer, VFS_DWORD dwBytesToWrite, VFS_DWORD * pBytesWritten);

// An Iteration Procedure (return VFS_FALSE to cancel Iteration).
typedef VFS_BOOL(*VFS_DirIterationProc) (const struct VFS_EntityInfo & Info, void *pParam);

// A List of Filter Names.
typedef std::vector < const class VFS_Filter *>VFS_FilterList;
typedef std::vector < VFS_String > VFS_FilterNameList;
typedef std::vector < VFS_String > VFS_RootPathList;
typedef std::vector < struct VFS_EntityInfo >VFS_EntityInfoList;
typedef std::map < VFS_String, VFS_String > VFS_FileNameMap;

//============================================================================
//    INTERFACE COMPONENT HEADERS
//============================================================================
//============================================================================
//    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    INTERFACE STRUCTURES / UTILITY CLASSES
//============================================================================
// A Filter for the VFS.
class VFS_Filter {
  public:
    // Constructor / Destructor.
    VFS_Filter() {
    } virtual ~ VFS_Filter() {
    }

    // Encoding / Decoding Procedures.
    virtual VFS_BOOL Encode(VFS_FilterReadProc Reader, VFS_FilterWriteProc Writer, const struct VFS_EntityInfo &DecodedInfo) const = 0;
    virtual VFS_BOOL Decode(VFS_FilterReadProc Reader, VFS_FilterWriteProc Writer, const struct VFS_EntityInfo &EncodedInfo) const = 0;

    // Filter Configuration Data Management.
    virtual VFS_BOOL LoadConfigData(VFS_FilterReadProc Reader) = 0;
    virtual VFS_BOOL SaveConfigData(VFS_FilterWriteProc Writer) const = 0;
    virtual VFS_DWORD GetConfigDataSize() const = 0;

    // Information.
    virtual VFS_PCSTR GetName() const = 0;
    virtual VFS_PCSTR GetDescription() const = 0;
};

// Information about a VFS Entity.
struct VFS_EntityInfo {
    // The Entity Type.
    VFS_EntityType eType;

    // Is the Entity archived (Archive files are NEVER archived)?
    VFS_BOOL bArchived;

    // The complete Path (including the Name) and the Name.
    VFS_String strPath;
    VFS_String strName;

    // The Size ( 0 for Directories ).
    VFS_LONG lSize;
};

//============================================================================
//    INTERFACE DATA DECLARATIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================
///////////////////////////////////////////////////////////////////////////////
// Basic VFS Interface (the error handling functions and the VFS_GetVersion() function may be called even if the VFS isn't initialized yet. The VFS_GetErrorString() function returns the string associated with VFS_ERROR_INVALID_ERROR_CODE if the parameter eError is invalid. You can't get Information about Archives using the VFS_GetEntityInfo() structure because it will report information about the virtual Directory the Archive represents instead).
///////////////////////////////////////////////////////////////////////////////
// Initialize / Shutdown the VFS.
VFS_BOOL VFS_Init();
VFS_BOOL VFS_Shutdown();
VFS_BOOL VFS_IsInit();

// Register / Unregister a Filter.
VFS_BOOL VFS_RegisterFilter(VFS_Filter * pFilter);
VFS_BOOL VFS_UnregisterFilter(VFS_Filter * pFilter);
VFS_BOOL VFS_UnregisterFilter(VFS_DWORD dwIndex);
VFS_BOOL VFS_UnregisterFilter(const VFS_String & strFilterName);
VFS_BOOL VFS_ExistsFilter(const VFS_String & strFilterName);
const VFS_Filter *VFS_GetFilter(const VFS_String & strFilterName);
VFS_DWORD VFS_GetNumFilters();
const VFS_Filter *VFS_GetFilter(VFS_DWORD dwIndex);
VFS_BOOL VFS_GetFilters(VFS_FilterList & Filters);
VFS_BOOL VFS_GetFilterNames(VFS_FilterNameList & FilterNames);

// Root Path Handling.
VFS_BOOL VFS_AddRootPath(const VFS_String & strRootPath);
VFS_BOOL VFS_RemoveRootPath(const VFS_String & strRootPath);
VFS_BOOL VFS_RemoveRootPath(VFS_DWORD dwIndex);
VFS_DWORD VFS_GetNumRootPaths();
VFS_BOOL VFS_GetRootPath(VFS_DWORD dwIndex, VFS_String & strRootPath);
VFS_BOOL VFS_GetRootPaths(VFS_RootPathList & RootPaths);

// Flush the VFS (close all unused Archives etc).
VFS_BOOL VFS_Flush();

// Information.
VFS_BOOL VFS_ExistsEntity(const VFS_String & strPath);
VFS_BOOL VFS_GetEntityInfo(const VFS_String & strPath, VFS_EntityInfo & Info);
VFS_WORD VFS_GetVersion();

// Error Handling.
VFS_ErrorCode VFS_GetLastError();
VFS_PCSTR VFS_GetErrorString(VFS_ErrorCode eError);

///////////////////////////////////////////////////////////////////////////////
// The File Interface (the file interface will try to create a file in each root path. If no root path has been added, the current directory will be used instead. You can't manipulate Archive Files.).
///////////////////////////////////////////////////////////////////////////////
// Create / Open / Close a File.
VFS_Handle VFS_File_Create(const VFS_String & strFileName, VFS_DWORD dwFlags);
VFS_Handle VFS_File_Open(const VFS_String & strFileName, VFS_DWORD dwFlags);
VFS_BOOL VFS_File_Close(VFS_Handle hFile);

// Read / Write from / to the File.
VFS_BOOL VFS_File_Read(VFS_Handle hFile, VFS_BYTE * pBuffer, VFS_DWORD dwToRead, VFS_DWORD * pRead = NULL);
VFS_BOOL VFS_File_Write(VFS_Handle hFile, const VFS_BYTE * pBuffer, VFS_DWORD dwToWrite, VFS_DWORD * pWritten = NULL);

// Direct File Reading / Writing.
VFS_BOOL VFS_File_ReadEntireFile(const VFS_String & strFileName, VFS_BYTE * pBuffer, VFS_DWORD dwToRead = VFS_INVALID_DWORD_VALUE, VFS_DWORD * pRead = NULL);
VFS_BOOL VFS_File_WriteEntireFile(const VFS_String & strFileName, const VFS_BYTE * pBuffer, VFS_DWORD dwToWrite, VFS_DWORD * pWritten = NULL);

// Positioning.
VFS_BOOL VFS_File_Seek(VFS_Handle hFile, VFS_LONG lPosition, VFS_SeekOrigin eOrigin = VFS_SET);
VFS_LONG VFS_File_Tell(VFS_Handle hFile);

// Sizing.
VFS_BOOL VFS_File_Resize(VFS_Handle hFile, VFS_LONG lSize);
VFS_LONG VFS_File_GetSize(VFS_Handle hFile);

// Information.
VFS_BOOL VFS_File_Exists(const VFS_String & strFileName);
VFS_BOOL VFS_File_GetInfo(const VFS_String & strFileName, VFS_EntityInfo & Info);
VFS_BOOL VFS_File_GetInfo(VFS_Handle hFile, VFS_EntityInfo & Info);

// File Management.
VFS_BOOL VFS_File_Delete(const VFS_String & strFileName);
VFS_BOOL VFS_File_Copy(const VFS_String & strFrom, const VFS_String & strTo);
VFS_BOOL VFS_File_Move(const VFS_String & strFrom, const VFS_String & strTo);
VFS_BOOL VFS_File_Rename(const VFS_String & strFrom, const VFS_String & strTo);	// pszTo has to be a single File Name without a Path.

///////////////////////////////////////////////////////////////////////////////
// The Archive Interface (Never provide a extension for the archive, instead, change the VFS_ARCHIVE_EXTENSION definition and recompile; You can only create archives in the first root path. You can't manipulate Archives. Each entry VFS_FileNameMap consists of the source file name and the file name in the archive, for instance "alpha/beta/gamma.txt" => "abg.txt").
///////////////////////////////////////////////////////////////////////////////
// Create an Archive.
VFS_BOOL VFS_Archive_CreateFromDirectory(const VFS_String & strArchiveFileName, const VFS_String & strDirName, const VFS_FilterNameList & UsedFilters = VFS_FilterNameList(), VFS_BOOL bRecursive = VFS_TRUE);
VFS_BOOL VFS_Archive_CreateFromFileList(const VFS_String & strArchiveFileName, const VFS_FileNameMap & Files, const VFS_FilterNameList & UsedFilters = VFS_FilterNameList());

// Extract an Archive / File.
VFS_BOOL VFS_Archive_Extract(const VFS_String & strArchiveFileName, const VFS_String & strTargetDir);
VFS_BOOL VFS_Archive_ExtractFile(const VFS_String & strArchiveFileName, const VFS_String & strFile, const VFS_String & strTargetFile);

// Information.
VFS_BOOL VFS_Archive_Exists(const VFS_String & strArchiveFileName);
VFS_BOOL VFS_Archive_GetInfo(const VFS_String & strArchiveFileName, VFS_EntityInfo & Info);
VFS_BOOL VFS_Archive_GetUsedFilters(const VFS_String & strArchiveFileName, VFS_FilterNameList & FilterNames);

// Archive Management.
VFS_BOOL VFS_Archive_Delete(const VFS_String & strArchiveFileName);

// Flush the Archive System.
VFS_BOOL VFS_Archive_Flush();

///////////////////////////////////////////////////////////////////////////////
// The Directory Interface (You can only create/delete standard directories in the first root path. You can't manipulate Dirs in Archives).
///////////////////////////////////////////////////////////////////////////////
// Directory Management.
VFS_BOOL VFS_Dir_Create(const VFS_String & strDirName, VFS_BOOL bRecursive = VFS_FALSE);	// Recursive mode would create a directory c:alphabeta even if alpha doesn't exist.
VFS_BOOL VFS_Dir_Delete(const VFS_String & strDirName, VFS_BOOL bRecursive = VFS_FALSE);	// Recursive mode would delete a directory c:alpha even if it contains files and/or subdirectories.

// Information.
VFS_BOOL VFS_Dir_Exists(const VFS_String & strDirName);
VFS_BOOL VFS_Dir_GetInfo(const VFS_String & strDirName, VFS_EntityInfo & Info);

// Iterate a Directory and call the iteration procedure for each 
VFS_BOOL VFS_Dir_Iterate(const VFS_String & strDirName, VFS_DirIterationProc pIterationProc, VFS_BOOL bRecursive = VFS_FALSE, void *pParam = NULL);

// Get the Contents of a Directory.
VFS_BOOL VFS_Dir_GetContents(const VFS_String & strDirName, VFS_EntityInfoList & EntityInfoList, VFS_BOOL bRecursive = VFS_FALSE);

///////////////////////////////////////////////////////////////////////////////
// The Utility Interface (You may call the File Name Management Functions even if the VFS isn't initialized yet).
///////////////////////////////////////////////////////////////////////////////
// File Name Management Functions.
VFS_BOOL VFS_Util_GetPath(const VFS_String & strFileName, VFS_String & strPath);
VFS_BOOL VFS_Util_GetName(const VFS_String & strFileName, VFS_String & strName);
VFS_BOOL VFS_Util_GetBaseName(const VFS_String & strFileName, VFS_String & strBaseName);
VFS_BOOL VFS_Util_GetExtension(const VFS_String & strFileName, VFS_String & strExtension);
VFS_BOOL VFS_Util_IsAbsoluteFileName(const VFS_String & strFileName);

//============================================================================
//    INTERFACE OBJECT CLASS DEFINITIONS
//============================================================================
//============================================================================
//    INTERFACE TRAILING HEADERS
//============================================================================

#endif				// __VFS_H__

