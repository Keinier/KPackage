//****************************************************************************
//**
//**    VFS_FILES.CPP
//**    File Interface Implementation
//**
//**	Project:	VFS
//**	Component:	Files
//**    Author:		Michael Walter
//**
//**	History:
//**		18.08.2001		Created (Michael Walter)
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
// The Open Files.
static FileMap g_OpenFiles;

//============================================================================
//    INTERFACE DATA
//============================================================================
static VFS_Handle ToHandlePlusStuff( IFile* pFile, VFS_String strAbsoluteFileName, VFS_String strRelativeFileName );
static VFS_Handle TryToOpen( const VFS_String&  strFileName, VFS_DWORD dwFlags );

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================
static VFS_Handle AddAndConvert( IFile* pFile, VFS_String strAbsoluteFileName )
{
	if( pFile == NULL )
		return VFS_INVALID_HANDLE_VALUE;

	// Add the File Name to the open Files Map.
	GetOpenFiles()[ ToLower( strAbsoluteFileName ) ] = pFile;

	return ( VFS_Handle )( VFS_DWORD )pFile;
}

static VFS_Handle TryToOpen( const VFS_String& strFileName, VFS_DWORD dwFlags )
{
	// Absolute?
	if( VFS_Util_IsAbsoluteFileName( strFileName ) )
	{
		// Already open?
		if( GetOpenFiles().find( ToLower( strFileName ) ) != GetOpenFiles().end() )
		{
			IFile* pFile = GetOpenFiles()[ ToLower( strFileName ) ];
			pFile->Add();
			return ( VFS_Handle )( VFS_DWORD )pFile;
		}

		// Exists a Standard File?
		if( CStdIOFile::Exists( strFileName ) )
			return AddAndConvert( CStdIOFile::Open( strFileName, dwFlags ), strFileName );

		// Try to open an Archive File.
		return AddAndConvert( CArchiveFile::Open( strFileName, dwFlags ), strFileName );
	}

	// For each Root Path...
	for( VFS_RootPathList::iterator iter = GetRootPaths().begin(); iter != GetRootPaths().end(); iter++ )
	{
		VFS_String strAbsoluteFileName = ToLower( WithoutTrailingSeparator( *iter, VFS_TRUE ) + VFS_PATH_SEPARATOR + strFileName );

		// Already open?
		if( GetOpenFiles().find( strAbsoluteFileName ) != GetOpenFiles().end() )
		{
			IFile* pFile = GetOpenFiles()[ strAbsoluteFileName ];
			pFile->Add();
			return ( VFS_Handle )( VFS_DWORD )pFile;
		}

		// Exists a Standard File?
		if( CStdIOFile::Exists( strAbsoluteFileName ) )
			return AddAndConvert( CStdIOFile::Open( strAbsoluteFileName, dwFlags ), strAbsoluteFileName );

		// Try to open an Archive File.
		if( CArchiveFile::Exists( strAbsoluteFileName ) )
			return AddAndConvert( CArchiveFile::Open( strAbsoluteFileName, dwFlags ), strAbsoluteFileName );
	}

    SetLastError( VFS_ERROR_NOT_FOUND );
	return VFS_INVALID_HANDLE_VALUE;
}

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================
// Create / Open / Close a File.

// Creates a File in the First Root Path.
VFS_Handle VFS_File_Create( const VFS_String& strFileName, VFS_DWORD dwFlags )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_INVALID_HANDLE_VALUE;
	}

	// No Root Paths specified yet?
	if( GetRootPaths().size() == 0 && !VFS_Util_IsAbsoluteFileName( strFileName ) )
	{
		SetLastError( VFS_ERROR_NO_ROOT_PATHS_DEFINED );
		return VFS_INVALID_HANDLE_VALUE;
	}

	// Make it absolute...
	VFS_String strAbsoluteFileName = ToLower( strFileName );
	if( !VFS_Util_IsAbsoluteFileName( strAbsoluteFileName ) )
		strAbsoluteFileName = ToLower( WithoutTrailingSeparator( GetRootPaths()[ 0 ], VFS_TRUE ) + VFS_PATH_SEPARATOR + strFileName );

	// Is such a File already open? Then just resize it.
	if( GetOpenFiles().find( strAbsoluteFileName ) != GetOpenFiles().end() )
	{
		IFile* pFile = GetOpenFiles()[ strAbsoluteFileName ];
		if( !pFile->Resize( 0 ) )
			return VFS_INVALID_HANDLE_VALUE;
		pFile->Add();
		return ( VFS_Handle )( VFS_DWORD )pFile;
	}

    // Try to create and return a StdIO File.	
	return AddAndConvert( CStdIOFile::Create( strAbsoluteFileName, dwFlags ), strAbsoluteFileName );
}

// Try to open a File with the specified Path (this function is way to big, hmm).
VFS_Handle VFS_File_Open( const VFS_String& strFileName, VFS_DWORD dwFlags )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_INVALID_HANDLE_VALUE;
	}

	// No Root Paths specified yet?
	if( GetRootPaths().size() == 0 && !VFS_Util_IsAbsoluteFileName( strFileName ) )
	{
		SetLastError( VFS_ERROR_NO_ROOT_PATHS_DEFINED );
		return VFS_INVALID_HANDLE_VALUE;
	}

	// Try to open the File.
	return TryToOpen( strFileName, dwFlags );
}

// Close the File.
VFS_BOOL VFS_File_Close( VFS_Handle hFile )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

    // Invalid Handle Value?
	VFS_BOOL bFound = VFS_FALSE;
	for( FileMap::iterator iter = GetOpenFiles().begin(); iter != GetOpenFiles().end(); iter++ )
	{
		// Found?
		if( ( *iter ).second == pFile )
		{
			bFound = VFS_TRUE;
			break;
		}
	}
	if( !bFound )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// If the File will be deleted afterwards, remove the File from the
	// Open Files Map.
	if( pFile->GetRefCount() == 1 )
	{
		FileMap::iterator iter = GetOpenFiles().find( pFile->GetFileName() );
		if( iter == GetOpenFiles().end() )
		{
			SetLastError( VFS_ERROR_GENERIC );
			return VFS_FALSE;
		}
		GetOpenFiles().erase( iter );
	}

	// Release the File.
	pFile->Release();

	return VFS_TRUE;
}

// Read / Write from / to the File.

// Read from the File.
VFS_BOOL VFS_File_Read( VFS_Handle hFile, VFS_BYTE* pBuffer, VFS_DWORD dwToRead, VFS_DWORD* pRead )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	return pFile->Read( pBuffer, dwToRead, pRead );
}

VFS_BOOL VFS_File_Write( VFS_Handle hFile, const VFS_BYTE* pBuffer, VFS_DWORD dwToWrite, VFS_DWORD* pWritten )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	return pFile->Write( pBuffer, dwToWrite, pWritten );
}

// Direct File Reading / Writing (it seems that it's less an optimized version than a version to simplify reading fixed-sized files).

// Read in the entire File at once.
VFS_BOOL VFS_File_ReadEntireFile( const VFS_String& strFileName, VFS_BYTE* pBuffer, VFS_DWORD dwToRead, VFS_DWORD* pRead )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Open the File.
	VFS_Handle hFile = VFS_File_Open( strFileName, VFS_READ );
	if( hFile == VFS_INVALID_HANDLE_VALUE )
		return VFS_FALSE;

    // Read in the File.
	if( !VFS_File_Read( hFile, pBuffer, dwToRead, pRead ) )
	{
		// Close the File.
		VFS_File_Close( hFile );

		return VFS_FALSE;
	}

    // Close the File.
	return VFS_File_Close( hFile );
}

// Write the entire File at once.
VFS_BOOL VFS_File_WriteEntireFile( const VFS_String& strFileName, const VFS_BYTE* pBuffer, VFS_DWORD dwToWrite, VFS_DWORD* pWritten )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// (Re)create the File.
	VFS_Handle hFile = VFS_File_Create( strFileName, VFS_WRITE );
	if( hFile == VFS_INVALID_HANDLE_VALUE )
		return VFS_FALSE;

    // Write the File.
	if( !VFS_File_Write( hFile, pBuffer, dwToWrite, pWritten ) )
	{
		// Close the File.
		VFS_File_Close( hFile );

		return VFS_FALSE;
	}

    // Close the File.
	return VFS_File_Close( hFile );
}

// Positioning.

// Seek in the File.
VFS_BOOL VFS_File_Seek( VFS_Handle hFile, VFS_LONG lPosition, VFS_SeekOrigin eOrigin )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	return pFile->Seek( lPosition, eOrigin );
}

// Return the current Position in the File.
VFS_LONG VFS_File_Tell( VFS_Handle hFile )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_INVALID_LONG_VALUE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_INVALID_LONG_VALUE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	return pFile->Tell();
}

// Sizing.

// Resize the File.
VFS_BOOL VFS_File_Resize( VFS_Handle hFile, VFS_LONG lSize )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	return pFile->Resize( lSize );
}

VFS_LONG VFS_File_GetSize( VFS_Handle hFile )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_INVALID_LONG_VALUE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_INVALID_LONG_VALUE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	return pFile->GetSize();
}

// Information.

// Determines whether a File with the specified File Name exists.
VFS_BOOL VFS_File_Exists( const VFS_String& strFileName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Just try to open the File.
	VFS_Handle hFile = VFS_File_Open( strFileName, VFS_READ );

    // Not found?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_NONE );	
		return VFS_FALSE;
	}

	// Close the File.
	return VFS_File_Close( hFile );
}

// Returns Information about the specified File.
VFS_BOOL VFS_File_GetInfo( const VFS_String& strFileName, VFS_EntityInfo& Info )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Try to open the File.
	VFS_Handle hFile = VFS_File_Open( strFileName, VFS_READ );
	if( hFile == VFS_INVALID_HANDLE_VALUE )
		return VFS_FALSE;

	// Try to get the File Information.
	if( !VFS_File_GetInfo( hFile, Info ) )
	{
		VFS_File_Close( hFile );
		return VFS_FALSE;
	}

    // Close the File.
	return VFS_File_Close( hFile );
}

// Returns Information about the File associated with the specified File Handle.
VFS_BOOL VFS_File_GetInfo( VFS_Handle hFile, VFS_EntityInfo& Info )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Handle Value?
	if( hFile == VFS_INVALID_HANDLE_VALUE )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	// Fill the Entity Information Structure.
	Info.bArchived = pFile->IsArchived();
	Info.eType = VFS_FILE;
	Info.lSize = pFile->GetSize();
	Info.strPath = pFile->GetFileName();
	return VFS_Util_GetName( Info.strPath, Info.strName );
}

// File Management.

// Delete the File with the specified File Name.
VFS_BOOL VFS_File_Delete( const VFS_String& strFileName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Try to open the file to get the absolute file name and to see if the file is still open
	// and if it's not in an Archive (VFS_WRITE would fail otherwise).
	VFS_Handle hFile = VFS_File_Open( strFileName, VFS_READ | VFS_WRITE );
	if( hFile == VFS_INVALID_HANDLE_VALUE )
		return VFS_FALSE;

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	// Check if there are still references to the File (but count ourself).
	if( pFile->GetRefCount() > 1 )
	{
		// Close the File.
		VFS_File_Close( hFile );

        SetLastError( VFS_ERROR_IN_USE );
		return VFS_FALSE;
	}

	// Get the absolute File Name.
	VFS_String strAbsoluteFileName = pFile->GetFileName();

	// Close the File.
	VFS_File_Close( hFile );

	// Try to delete the File.
	if( !VFS_UNLINK( strAbsoluteFileName ) )
	{
		SetLastError( VFS_ERROR_PERMISSION_DENIED );
		return VFS_FALSE;
	}

	return VFS_TRUE;
}

// Copy the specified File.
VFS_BOOL VFS_File_Copy( const VFS_String& strFrom, const VFS_String& strTo )
{
	static VFS_BYTE Chunk[ FILE_COPY_CHUNK_SIZE ];

	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Open the Files.
	VFS_Handle hIn = VFS_File_Open( strFrom, VFS_READ );
	if( hIn == VFS_INVALID_HANDLE_VALUE )
		return VFS_FALSE;

	VFS_Handle hOut = VFS_File_Create( strTo, VFS_WRITE );
	if( hIn == VFS_INVALID_HANDLE_VALUE )
	{
		// Close the Input File.
		VFS_File_Close( hIn );

		return VFS_FALSE;
	}

	// Until EOF...
	VFS_DWORD dwRead, dwWritten;
	do
	{
		// Read in a Chunk.
		if( !VFS_File_Read( hIn, Chunk, FILE_COPY_CHUNK_SIZE, &dwRead ) )
		{
			// Close the Files.
			VFS_File_Close( hOut );
			VFS_File_Close( hIn );

			return VFS_FALSE;
		}

		// Write the Chunk.
		if( !VFS_File_Write( hOut, Chunk, dwRead, &dwWritten ) )
		{
			// Close the Files.
			VFS_File_Close( hOut );
			VFS_File_Close( hIn );

			return VFS_FALSE;
		}

        // Unknown Error?
		if( dwWritten < dwRead )
		{
			// Close the Files.
			VFS_File_Close( hOut );
			VFS_File_Close( hIn );

			return VFS_FALSE;
		}
	}
	while( dwRead > 0 );

	// Close the Files.
	if( !VFS_File_Close( hOut ) )
	{
		// Close the Input File.
		VFS_File_Close( hIn );

		return VFS_FALSE;
	}

	return VFS_File_Close( hIn );
}

// Move the specified File.
VFS_BOOL VFS_File_Move( const VFS_String& strFrom, const VFS_String& strTo )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Try to copy the file.
	if( !VFS_File_Copy( strFrom, strTo ) )
		return VFS_FALSE;

	// Try to delete the Source File.
	return VFS_File_Delete( strFrom );
}

// Rename the specified File.
VFS_BOOL VFS_File_Rename( const VFS_String& strFrom, const VFS_String& strTo )				// pszTo has to be a single File Name without a Path.
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Try to open the file to get the absolute file name and to see if the file is still open
	// and if it's not in an Archive (VFS_WRITE would fail otherwise).
	VFS_Handle hFile = VFS_File_Open( strFrom, VFS_READ | VFS_WRITE );
	if( hFile == VFS_INVALID_HANDLE_VALUE )
		return VFS_FALSE;

	// Get the File Pointer.
	IFile* pFile = ( IFile* )( VFS_DWORD )hFile;

	// Check if there are still references to the File (but count ourself).
	if( pFile->GetRefCount() > 1 )
	{
		// Close the File.
		VFS_File_Close( hFile );

        SetLastError( VFS_ERROR_IN_USE );
		return VFS_FALSE;
	}

	// Get the absolute File Name.
	VFS_String strAbsoluteFileName = pFile->GetFileName();

	// Close the File.
	VFS_File_Close( hFile );

	// Make the Target Name absolute.
	VFS_String strAbsoluteTo;
	VFS_Util_GetPath( strAbsoluteFileName, strAbsoluteTo );
	strAbsoluteTo = WithoutTrailingSeparator( strAbsoluteTo, VFS_TRUE ) + VFS_PATH_SEPARATOR + strTo;

	// Try to rename the File.
	if( !VFS_RENAME( strAbsoluteFileName, strAbsoluteTo ) )
	{
		SetLastError( VFS_ERROR_PERMISSION_DENIED );
		return VFS_FALSE;
	}

	return VFS_TRUE;
}

// Internal Stuff.

// Return a Map containing all Open Files.
FileMap& GetOpenFiles()
{
	return g_OpenFiles;
}

//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================