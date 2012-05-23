//****************************************************************************
//**
//**    VFS_BASIC.CPP
//**    No Description
//**
//**	Project:	VFS
//**	Component:	Basic
//**    Author:		Michael Walter
//**
//**	History:
//**		15.08.2001		Created (Michael Walter)
//****************************************************************************

//============================================================================
//    IMPLEMENTATION HEADERS
//============================================================================
#include "VFS_Implementation.h"

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE STRUCTURES / UTILITY CLASSES
//============================================================================
//============================================================================
//    IMPLEMENTATION REQUIRED EXTERNAL REFERENCES (AVOID)
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================
// The active Archive (the Archive whose Filter Data is applied).
CArchive* CArchive::m_pActive = NULL;

//============================================================================
//    INTERFACE DATA
//============================================================================
// From & To Buffer.
vector< VFS_BYTE > g_FromBuffer, g_ToBuffer;
VFS_DWORD g_FromPos;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================
//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
// --- Archive Class ---
// Parse the Archive.
VFS_BOOL CArchive::Parse()
{
	// Read in the Archive Header.
	ARCHIVE_HEADER RawHeader;
	if( !VFS_File_Read( m_hFile, ( VFS_BYTE* ) &RawHeader, sizeof( ARCHIVE_HEADER ) ) )
		return VFS_FALSE;

	m_Header.dwDataOffset = sizeof( ARCHIVE_HEADER ) +
							RawHeader.dwNumFilters * sizeof( ARCHIVE_FILTER ) +
							RawHeader.dwNumDirs * sizeof( ARCHIVE_DIR ) +
							RawHeader.dwNumFiles * sizeof( ARCHIVE_FILE );
	m_Header.dwFileDataOffset = m_Header.dwDataOffset;

	// Read in the Filters.
	VFS_DWORD dwIndex;
	for( dwIndex = 0; dwIndex < RawHeader.dwNumFilters; dwIndex++ )
	{
		ARCHIVE_FILTER RawFilter;
		if( !VFS_File_Read( m_hFile, ( VFS_BYTE* ) &RawFilter, sizeof( ARCHIVE_FILTER ) ) )
			return VFS_FALSE;

		// Get the Filter for the Name.
		VFS_Filter* pFilter = ( VFS_Filter* ) VFS_GetFilter( RawFilter.szName );
		if( pFilter == NULL )
			return VFS_FALSE;

        // Increase the File Data Offset.
		m_Header.dwFileDataOffset += pFilter->GetConfigDataSize();

		m_Header.Filters.push_back( pFilter );
	}

	// Read in the Dirs.
	for( dwIndex = 0; dwIndex < RawHeader.dwNumDirs; dwIndex++ )
	{
		ARCHIVE_DIR RawDir;
		if( !VFS_File_Read( m_hFile, ( VFS_BYTE* ) &RawDir, sizeof( ARCHIVE_DIR ) ) )
			return VFS_FALSE;

		ArchiveDir Dir;
		Dir.dwParentDirIndex = RawDir.dwParentIndex;
		Dir.strName = RawDir.szName;

		m_Header.Dirs.push_back( Dir );
	}

	// Calculate the Full Name.
	for( dwIndex = 0; dwIndex < RawHeader.dwNumDirs; dwIndex++ )
	{
		ArchiveDir* pBase = &m_Header.Dirs[ dwIndex ];
		ArchiveDir* pDir = pBase;
		while( pDir->dwParentDirIndex != DIR_INDEX_ROOT )
		{
			pDir = &m_Header.Dirs[ pDir->dwParentDirIndex ];
			pBase->strName = pDir->strName + VFS_PATH_SEPARATOR + pBase->strName;
		}

		m_Header.DirHash[ m_Header.Dirs[ dwIndex ].strName ] = dwIndex;
	}

	// Read in the Filter Data.
	VFS_DWORD dwPos = VFS_File_Tell( m_hFile );
	if( !Activate() )
		return VFS_FALSE;
	VFS_File_Seek( m_hFile, dwPos, VFS_SET );

	// Read in the Files.
	VFS_DWORD dwDataOffset = m_Header.dwFileDataOffset;
	for( dwIndex = 0; dwIndex < RawHeader.dwNumFiles; dwIndex++ )
	{
		ARCHIVE_FILE RawFile;
		if( !VFS_File_Read( m_hFile, ( VFS_BYTE* ) &RawFile, sizeof( ARCHIVE_FILE ) ) )
			return VFS_FALSE;

		ArchiveFile File;
		File.strName = RawFile.szName;
		File.dwDirIndex = RawFile.dwDirIndex;
		if( File.dwDirIndex != DIR_INDEX_ROOT )
			File.strName = m_Header.Dirs[ File.dwDirIndex ].strName + VFS_PATH_SEPARATOR + File.strName;
		File.dwCompressedSize = RawFile.dwCompressedSize;
		File.dwUncompressedSize = RawFile.dwUncompressedSize;
		File.dwDataOffset = dwDataOffset;
		dwDataOffset += File.dwCompressedSize;
		m_Header.Files.push_back( File );

		m_Header.FileHash[ m_Header.Files[ dwIndex ].strName ] = dwIndex;
	}

	return VFS_TRUE;
}

// Constructor / Destructor.
CArchive::CArchive( VFS_String strAbsoluteFileName )
{
	m_strFileName = strAbsoluteFileName;

	// Try to open the Archive.
	m_hFile = VFS_File_Open( m_strFileName, VFS_READ );
	if( m_hFile == VFS_INVALID_HANDLE_VALUE )
		return;

	// Parse the Archive:
	if( !Parse() )
	{
		VFS_File_Close( m_hFile );
		m_hFile = VFS_INVALID_HANDLE_VALUE;
		SetLastError( VFS_ERROR_INVALID_ARCHIVE_FORMAT );
		return;
	}
}

CArchive::~CArchive()
{
	if( m_hFile != VFS_INVALID_HANDLE_VALUE )
		VFS_File_Close( m_hFile );
}

// Is this Archive valid?
VFS_BOOL CArchive::IsValid() const
{
	return m_hFile != VFS_INVALID_HANDLE_VALUE;
}

// The File Handle.
VFS_Handle CArchive::GetFile() const
{
	return m_hFile;
}

// The Archive Header.
const ArchiveHeader* CArchive::GetHeader() const
{
	return &m_Header;
}

// Get the Reference Count (i.e. the Sum of the Reference Counts of all Open Files of this Archive).
VFS_DWORD CArchive::GetRefCount() const
{
	VFS_DWORD dwRefCount = 0;
	for( FileMap::iterator iter = GetOpenFiles().begin(); iter != GetOpenFiles().end(); iter++ )
	{
		IFile* pFile = ( *iter ).second;
		if( pFile->IsArchived() )
		{
			CArchiveFile* pArchiveFile = ( CArchiveFile* )pFile;
			if( pArchiveFile->GetArchive() == this )
				dwRefCount++;
		}
	}
	return dwRefCount;
}

// Directory Stuff.
VFS_BOOL CArchive::ContainsDir( const VFS_String& strDirName ) const
{
	return m_Header.DirHash.find( ToLower( strDirName ) ) != m_Header.DirHash.end();
}

VFS_BOOL CArchive::IterateDir( const VFS_String& strDirName, VFS_DirIterationProc pIterationProc, VFS_BOOL bRecursive, void* pParam ) const
{
	// Get the dir for the name.
	VFS_DWORD dwDirIndex;
	VFS_String strDir = WithoutTrailingSeparator( ToLower( strDirName ), VFS_TRUE );
	if( strDir != VFS_TEXT( "" ) )
	{
		ArchiveDirMap::const_iterator iter = m_Header.DirHash.find( ToLower( strDir ) );
		if( iter == m_Header.DirHash.end() )
		{
			SetLastError( VFS_ERROR_NOT_FOUND );
			return VFS_FALSE;
		}
		dwDirIndex = ( *iter ).second;
	}
	else
		dwDirIndex = DIR_INDEX_ROOT;

	// Iterate for all Dirs in the Dir and call IterateDir() on these (if in Recursive Mode).
	VFS_DWORD dwIndex;
	for( dwIndex = 0; dwIndex < m_Header.Dirs.size(); dwIndex++ )
	{
		if( m_Header.Dirs[ dwIndex ].dwParentDirIndex != dwDirIndex )
			continue;

		VFS_EntityInfo Info;
		Info.bArchived = VFS_TRUE;
		Info.eType = VFS_DIR;
		Info.lSize = 0;
		Info.strPath = GetFileNameWithoutExtension() + VFS_PATH_SEPARATOR + m_Header.Dirs[ dwIndex ].strName;
		if( !VFS_Util_GetName( Info.strPath, Info.strName ) )
			return VFS_FALSE;

		if( !pIterationProc( Info, pParam ) )
			return VFS_TRUE;

        if( bRecursive && !IterateDir( m_Header.Dirs[ dwIndex ].strName, pIterationProc, bRecursive, pParam ) )
			return VFS_FALSE;
	}

	// Iterate for all Files in the Dir.
	for( dwIndex = 0; dwIndex < m_Header.Files.size(); dwIndex++ )
	{
		if( m_Header.Files[ dwIndex ].dwDirIndex != dwDirIndex )
			continue;

		VFS_EntityInfo Info;
		Info.bArchived = VFS_TRUE;
		Info.eType = VFS_FILE;
		Info.lSize = m_Header.Files[ dwIndex ].dwUncompressedSize;
		Info.strPath = GetFileNameWithoutExtension() + VFS_PATH_SEPARATOR + m_Header.Files[ dwIndex ].strName;
		if( !VFS_Util_GetName( Info.strPath, Info.strName ) )
			return VFS_FALSE;

		if( !pIterationProc( Info, pParam ) )
			return VFS_TRUE;
	}

	return VFS_TRUE;
}

// File Stuff.
VFS_BOOL CArchive::ContainsFile( const VFS_String& strFileName ) const
{
	return strFileName == VFS_TEXT( "" ) ||							// Root Directory
		   m_Header.FileHash.find( ToLower( strFileName ) ) != m_Header.FileHash.end();
}

const ArchiveFile* CArchive::GetFile( const VFS_String& strFileName ) const
{
	if( m_Header.FileHash.find( ToLower( strFileName ) ) == m_Header.FileHash.end() )
	{
		SetLastError( VFS_ERROR_NOT_FOUND );
		return NULL;
	}

	ArchiveFileMap& Hash = const_cast< ArchiveFileMap& >( m_Header.FileHash );
    VFS_DWORD dwIndex = Hash[ ToLower( strFileName ) ];
	return &m_Header.Files[ dwIndex ];
}

// Extraction.
VFS_BOOL CArchive::Extract( const VFS_String& strTargetDir ) const
{
	if( !VFS_Dir_Exists( strTargetDir ) )
		if( !VFS_Dir_Create( strTargetDir, VFS_TRUE ) )
			return VFS_FALSE;
	VFS_EntityInfo Info;
	if( !VFS_Dir_GetInfo( strTargetDir, Info ) )
		return VFS_FALSE;
	VFS_String strTarget = WithoutTrailingSeparator( Info.strPath, VFS_TRUE ) + VFS_PATH_SEPARATOR;

	// Activate ourselves.
	if( !const_cast< CArchive* >( this )->Activate() )
		return VFS_FALSE;

	// Extract all Dirs.
	VFS_DWORD dwIndex;
	for( dwIndex = 0; dwIndex <	m_Header.Dirs.size(); dwIndex++ )
	{
		VFS_String strDirName = strTarget + m_Header.Dirs[ dwIndex ].strName;
		if( !VFS_Dir_Exists( strDirName ) )
			if( !VFS_Dir_Create( strDirName, VFS_TRUE ) )
				return VFS_FALSE;
	}

	// Extract all Files.
	for( dwIndex = 0; dwIndex <	m_Header.Files.size(); dwIndex++ )
	{
		VFS_String strFileName = strTarget + m_Header.Files[ dwIndex ].strName;

		// Read in the File.
		if( !VFS_File_Seek( m_hFile, m_Header.Files[ dwIndex ].dwDataOffset, VFS_SET ) )
			return VFS_FALSE;
		g_FromBuffer.resize( m_Header.Files[ dwIndex ].dwCompressedSize );
		if( !VFS_File_Read( m_hFile, &*g_FromBuffer.begin(), m_Header.Files[ dwIndex ].dwCompressedSize ) )
			return VFS_FALSE;

		// Apply the Filters.
		VFS_EntityInfo Info;
		Info.bArchived = VFS_TRUE;
		Info.eType = VFS_FILE;
		Info.lSize = m_Header.Files[ dwIndex ].dwCompressedSize;
		Info.strPath = GetFileNameWithoutExtension() + VFS_PATH_SEPARATOR + m_Header.Files[ dwIndex ].strName;
		VFS_Util_GetName( Info.strPath, Info.strName );

		for( VFS_DWORD dwFilter = 0; dwFilter != m_Header.Filters.size(); dwFilter++ )
		{
			g_FromPos = 0;
			if( !m_Header.Filters[ dwFilter ]->Decode( Reader, Writer, Info ) )
			{
				VFS_ErrorCode eError = VFS_GetLastError();
				if( eError == VFS_ERROR_NONE )
					eError = VFS_ERROR_GENERIC;
				SetLastError( eError );
				return VFS_FALSE;
			}
			g_FromBuffer = g_ToBuffer;
			g_ToBuffer.clear();
		}

		// Create the Target File.
		VFS_Handle hFile = VFS_File_Create( strFileName, VFS_WRITE );
		if( hFile == VFS_INVALID_HANDLE_VALUE )
			return VFS_FALSE;

		// Write the Data.
		if( !VFS_File_Write( hFile, &*g_FromBuffer.begin(), ( VFS_DWORD )g_FromBuffer.size() ) )
			return VFS_FALSE;

		// Close the Target File.
		if( !VFS_File_Close( hFile ) )
			return VFS_FALSE;
	}

	return VFS_TRUE;
}

// Activation.
VFS_BOOL CArchive::Activate()
{
	if( m_pActive == this )
		return VFS_TRUE;

	VFS_DWORD dwDataOffset = m_Header.dwDataOffset;
	if( !VFS_File_Seek( m_hFile, dwDataOffset, VFS_SET ) )
		return VFS_FALSE;
	FilterList::const_iterator iter;
	for( iter = m_Header.Filters.begin(); iter != m_Header.Filters.end(); iter++ )
	{
		g_FromBuffer.resize( ( *iter )->GetConfigDataSize() );
		if( !VFS_File_Read( m_hFile, &*g_FromBuffer.begin(), ( VFS_DWORD )g_FromBuffer.size() ) )
			return VFS_FALSE;
		dwDataOffset += ( *iter )->GetConfigDataSize();
		g_FromPos = 0;
		( *iter )->LoadConfigData( Reader );
	}

	m_pActive = this;

	return VFS_TRUE;
}

// Open an Archive.
CArchive* CArchive::Open( const VFS_String& strAbsoluteFileName )
{
	// Try to open the Archive.
	CArchive* pArchive = new CArchive( CheckExtension( strAbsoluteFileName ) );

	// Invalid Archive?
	if( !pArchive->IsValid() )
	{
		delete pArchive;
		return NULL;
	}

	return pArchive;
}

// Check the File Existence.
VFS_BOOL CArchive::Exists( const VFS_String& strAbsoluteFileName )
{
	return VFS_EXISTS( CheckExtension( strAbsoluteFileName ) );
}

// Check / modify the Archive Extension.
VFS_String CArchive::CheckExtension( VFS_String strFileName )
{
	static const VFS_String RIGHT( VFS_String( VFS_TEXT( "." ) ) + VFS_ARCHIVE_FILE_EXTENSION );

	if( strFileName.size() < RIGHT.size() ||
		ToLower( strFileName.substr( strFileName.size() - RIGHT.size(), RIGHT.size() ) ) !=
			ToLower( RIGHT ) )
		return strFileName + VFS_TEXT( "." ) + VFS_ARCHIVE_FILE_EXTENSION;
	return strFileName;
}

// Internal Stuff.
VFS_BOOL Reader( VFS_BYTE* pBuffer, VFS_DWORD dwBytesToRead, VFS_DWORD* pBytesRead )
{
	if( pBuffer == NULL )
		return VFS_FALSE;

	if( dwBytesToRead > 0 && g_FromPos >= g_FromBuffer.size() )
		return VFS_FALSE;

	if( pBytesRead )
		*pBytesRead = 0;
	for( VFS_DWORD i = 0; ( i < dwBytesToRead ) && ( g_FromPos < g_FromBuffer.size() ); i++ )
	{
		pBuffer[ i ] = g_FromBuffer[ g_FromPos ];
		g_FromPos++;
		if( pBytesRead )
			( *pBytesRead )++;
	}

	return VFS_TRUE;
}

VFS_BOOL Writer( const VFS_BYTE* pBuffer, VFS_DWORD dwBytesToWrite, VFS_DWORD* pBytesWritten )
{
	if( pBuffer == NULL )
		return VFS_FALSE;

	for( VFS_DWORD i = 0; i < dwBytesToWrite; i++ )
	{
		g_ToBuffer.push_back( pBuffer[ i ] );
	}

	if( pBytesWritten )
		*pBytesWritten = dwBytesToWrite;

	return VFS_TRUE;
}