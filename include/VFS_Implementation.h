//****************************************************************************
//**
//**    VFS_IMPLEMENTATION.H
//**    Header - No Description
//**
//**    Project:        VFS
//**    Component:      VFS/Implementation
//**    Author:         Michael Walter
//**                    Keinier Caboverde
//**
//**    History:
//**            15.08.2001              Created (Michael Walter)
//**            22.05.2012      Unix Ported(Keinier Caboverde)
//**
//**    Notes:
//**            Bold Names = File Structures.
//**
//****************************************************************************
#ifndef VFS_VFS_IMPLEMENTATION_H
#define VFS_VFS_IMPLEMENTATION_H

//============================================================================
//    INTERFACE REQUIRED HEADERS
//============================================================================
#include "VFS.h"
#include <cassert>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
// The Archive ID.
static const VFS_BYTE ARCHIVE_ID[4] = { 'V', 'F', 'S', '1' };

// The File Copy Chunk Size (at the moment 10K). Feel free to modify it to a better value.
static const VFS_DWORD FILE_COPY_CHUNK_SIZE = 10 * 1024;

// Parent = Root Directory (which hasn't an Entry).
static const VFS_DWORD DIR_INDEX_ROOT = 0xFFFFFFFF;

// A Filter Name->Filter Point Map.
typedef map < VFS_String, VFS_Filter * >FilterMap;

// Map Types.
typedef map < VFS_String, class CArchive * >ArchiveMap;	// Absolute Archive File Name -> Archive Pointer.
typedef map < VFS_String, class IFile * >FileMap;	// Absolute File Name -> File Pointer.

//============================================================================
//    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    INTERFACE STRUCTURES / UTILITY CLASSES
//============================================================================
// --- The File Structures ---
// The Archive Header.
#pragma pack( push, 1 )
struct ARCHIVE_HEADER {
    VFS_BYTE ID[4];
    VFS_WORD wVersion;
    VFS_DWORD dwNumFilters;
    VFS_DWORD dwNumDirs;
    VFS_DWORD dwNumFiles;
};

// The Filter Structure.
struct ARCHIVE_FILTER {
    VFS_CHAR szName[VFS_MAX_NAME_LENGTH];
};

// The Dir Structure.
struct ARCHIVE_DIR {
    VFS_CHAR szName[VFS_MAX_NAME_LENGTH];
    VFS_DWORD dwParentIndex;
};

// The File Structure.
struct ARCHIVE_FILE {
    VFS_CHAR szName[VFS_MAX_NAME_LENGTH];
    VFS_DWORD dwDirIndex;
    VFS_DWORD dwUncompressedSize;
    VFS_DWORD dwCompressedSize;
};
#pragma pack( pop )

// --- The Memory Structures ---
typedef vector < VFS_Filter * >FilterList;

struct ArchiveDir {
    VFS_String strName;
    VFS_DWORD dwParentDirIndex;
};
typedef map < VFS_String, VFS_DWORD > ArchiveDirMap;	// Absolute (!) Dir Name -> Dir Index.
typedef vector < ArchiveDir > ArchiveDirList;

struct ArchiveFile {
    VFS_String strName;
    VFS_DWORD dwDirIndex;
    VFS_DWORD dwDataOffset;
    VFS_DWORD dwCompressedSize;
    VFS_DWORD dwUncompressedSize;
};
typedef map < VFS_String, VFS_DWORD > ArchiveFileMap;	// Absolute (!) File Name -> File Index.
typedef vector < ArchiveFile > ArchiveFileList;

struct ArchiveHeader {
    FilterList Filters;
    ArchiveDirList Dirs;
    ArchiveDirMap DirHash;
    ArchiveFileList Files;
    ArchiveFileMap FileHash;
    VFS_DWORD dwDataOffset;
    VFS_DWORD dwFileDataOffset;
};

// --- Classes for Archive Access ---
class CArchive {
    VFS_String m_strFileName;
    VFS_Handle m_hFile;
    ArchiveHeader m_Header;
    static CArchive *m_pActive;

    // Parse the Archive.
    VFS_BOOL Parse();

  public:
    // Constructor / Destructor.
     CArchive(VFS_String strAbsoluteFileName);
     virtual ~ CArchive();

    // Information.
    const VFS_String & GetFileName() const {
	return m_strFileName;
    } VFS_String GetFileNameWithoutExtension() const {
	return m_strFileName.substr(0, m_strFileName.size() - VFS_String(VFS_ARCHIVE_FILE_EXTENSION).size() - 1);
    }
    // Is this Archive valid? 
    VFS_BOOL IsValid() const;

    // The File Handle.
    VFS_Handle GetFile() const;

    // The Archive Header.
    const ArchiveHeader *GetHeader() const;

    // Get the Reference Count (i.e. the Sum of the Reference Counts of all Open Files of this Archive).
    VFS_DWORD GetRefCount() const;

    // Directory Stuff.
    VFS_BOOL ContainsDir(const VFS_String & strDirName) const;
    VFS_BOOL IterateDir(const VFS_String & strDirName, VFS_DirIterationProc pIterationProc, VFS_BOOL bRecursive, void *pParam) const;

    // File Stuff.
    VFS_BOOL ContainsFile(const VFS_String & strFileName) const;
    const ArchiveFile *GetFile(const VFS_String & strFileName) const;

    // Extraction.
    VFS_BOOL Extract(const VFS_String & strTargetDir) const;

    // Activation.
    VFS_BOOL Activate();

    // Open / Check the Existence of an Archive.
    static CArchive *Open(const VFS_String & strAbsoluteFileName);
    static VFS_BOOL Exists(const VFS_String & strAbsoluteFileName);

    // Check / modify the Archive Extension.
    static VFS_String CheckExtension(VFS_String strFileName);
};

// --- Classes for File Access ---
class IFile {
    VFS_DWORD m_dwReferenceCount;
    VFS_String m_strFileName;

  protected:
    // Set the File Name of this IFile.
    void SetFileName(const VFS_String & strFileName);

  public:
    // Constructor / Destructor.
     IFile() {
	m_dwReferenceCount = 1;
	SetFileName(VFS_TEXT("(not set)"));
    } IFile(const VFS_String & strFileName) {
	m_dwReferenceCount = 1;
	SetFileName(strFileName);
    }
    virtual ~ IFile() {
    }

    // Information.
    const VFS_String & GetFileName() const {
	return m_strFileName;
    }
    // Add / Release. 
    void Add() {
	++m_dwReferenceCount;
    }
    void Release() {
	if (--m_dwReferenceCount == 0)
	    delete this;
    }
    VFS_DWORD GetRefCount() const {
	return m_dwReferenceCount;
    }
    // Read / Write. 
    virtual VFS_BOOL Read(VFS_BYTE * pBuffer, VFS_DWORD dwToRead, VFS_DWORD * pRead) = 0;
    virtual VFS_BOOL Write(const VFS_BYTE * pBuffer, VFS_DWORD dwToWrite, VFS_DWORD * pWritten) = 0;

    // Seek / Tell.
    virtual VFS_BOOL Seek(VFS_LONG lPosition, VFS_SeekOrigin eOrigin) = 0;
    virtual VFS_LONG Tell() const = 0;

    // Sizing.
    virtual VFS_BOOL Resize(VFS_LONG lSize) = 0;
    virtual VFS_LONG GetSize() const = 0;

    // Information.
    virtual VFS_BOOL IsArchived() const = 0;
};

class CStdIOFile:public IFile {
    FILE *m_pFile;
    VFS_BOOL m_bReadOnly;

  public:
    // Constructor / Destructor.
     CStdIOFile(const VFS_String & strAbsoluteFileName, VFS_BOOL bOpen);
     virtual ~ CStdIOFile();

    // Is the File valid?
    VFS_BOOL IsValid() const;

    // Read / Write.
    VFS_BOOL Read(VFS_BYTE * pBuffer, VFS_DWORD dwToRead, VFS_DWORD * pRead);
    VFS_BOOL Write(const VFS_BYTE * pBuffer, VFS_DWORD dwToWrite, VFS_DWORD * pWritten);

    // Seek / Tell.
    VFS_BOOL Seek(VFS_LONG lPosition, VFS_SeekOrigin eOrigin);
    VFS_LONG Tell() const;

    // Sizing.
    VFS_BOOL Resize(VFS_LONG lSize);
    VFS_LONG GetSize() const;

    // Information.
    VFS_BOOL IsArchived() const {
	return VFS_FALSE;
    }
    // Open / Create a StdIOFile.
    static IFile *Open(const VFS_String & strAbsoluteFileName, VFS_DWORD dwFlags);
    static IFile *Create(const VFS_String & strAbsoluteFileName, VFS_DWORD dwFlags);
    static VFS_BOOL Exists(const VFS_String & strAbsoluteFileName);
};

class CArchiveFile:public IFile {
    const CArchive *m_pArchive;
    const ArchiveFile *m_pArchiveFile;
     vector < VFS_BYTE > m_Data;
    VFS_DWORD m_dwPos;

  public:
    // Constructor / Destructor.
     CArchiveFile(const CArchive * pArchive, const VFS_String & strFileName, VFS_BOOL bOpen);
     virtual ~ CArchiveFile();

    // Information.
    const CArchive *GetArchive() {
	return m_pArchive;
    }
    // Is the File valid? 
    VFS_BOOL IsValid() const;

    // Read / Write.
    VFS_BOOL Read(VFS_BYTE * pBuffer, VFS_DWORD dwToRead, VFS_DWORD * pRead);
    VFS_BOOL Write(const VFS_BYTE * pBuffer, VFS_DWORD dwToWrite, VFS_DWORD * pWritten);

    // Seek / Tell.
    VFS_BOOL Seek(VFS_LONG lPosition, VFS_SeekOrigin eOrigin);
    VFS_LONG Tell() const;

    // Sizing.
    VFS_BOOL Resize(VFS_LONG lSize);
    VFS_LONG GetSize() const;

    // Information.
    VFS_BOOL IsArchived() const {
	return VFS_TRUE;
    }
    // Open / Create an Archive File. 
    static IFile *Open(const VFS_String & strAbsoluteFileName, VFS_DWORD dwFlags);
    static VFS_BOOL Exists(const VFS_String & strAbsoluteFileName, CArchive ** ppArchive = NULL, VFS_String * pFileName = NULL);
};

//============================================================================
//    INTERFACE DATA DECLARATIONS
//============================================================================
// From & To Buffer Stuff.
extern vector < VFS_BYTE > g_FromBuffer, g_ToBuffer;
extern VFS_DWORD g_FromPos;

//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================
// Is the VFS initialized?
const VFS_BOOL & IsInit();

// Internal Error Handling.
void SetLastError(VFS_ErrorCode eError);

// Get the open Files and Archives.
FileMap & GetOpenFiles();
ArchiveMap & GetOpenArchives();

// Get the Root Path List.
VFS_RootPathList & GetRootPaths();

// (Always) removes a trailing Path Separator Char.
VFS_String WithoutTrailingSeparator(const VFS_String & strFileName, VFS_BOOL bForce);

// Determines if the specified File Name represents a Root Directory.
VFS_BOOL IsRootDir(const VFS_String & strFileName);

// Makes a String lower-cased.
VFS_String ToLower(const VFS_String & strString);

// Array Reader and Writer.
VFS_BOOL Reader(VFS_BYTE * pBuffer, VFS_DWORD dwBytesToRead, VFS_DWORD * pBytesRead);
VFS_BOOL Writer(const VFS_BYTE * pBuffer, VFS_DWORD dwBytesToWrite, VFS_DWORD * pBytesWritten);

//============================================================================
//    INTERFACE CLASS IMPLEMENTATIONS
//============================================================================
inline void IFile::SetFileName(const VFS_String & strFileName)
{
    m_strFileName = ToLower(strFileName);
}

//============================================================================
//    INTERFACE TRAILING HEADERS
//============================================================================

#endif

