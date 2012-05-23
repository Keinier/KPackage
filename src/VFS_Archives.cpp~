//****************************************************************************
//**
//**    VFS_ARCHIVES.CPP
//**    Archive Interface Implementation
//**
//**	Project:	VFS
//**	Component:	Archives
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
// The Open Archives.
static ArchiveMap g_OpenArchives;

//============================================================================
//    INTERFACE DATA
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================
// Create an Archive.

// Create an Archive from the specified Source Directory.
VFS_BOOL VFS_Archive_CreateFromDirectory( const VFS_String& strArchiveFileName, const VFS_String& strDirName, const VFS_FilterNameList& UsedFilters, VFS_BOOL bRecursive )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Get the Directory Contents.
	VFS_EntityInfoList EntityInfos;
	if( !VFS_Dir_GetContents( strDirName, EntityInfos, bRecursive ) )
		return VFS_FALSE;

	// Get the absolute Path of the Directory.
	VFS_EntityInfo Info;
	if( !VFS_Dir_GetInfo( strDirName, Info ) )
		return VFS_FALSE;

	// Get the Start Position for the In-Archive Name.
	VFS_INT nStart = ( VFS_INT )Info.strPath.size();
	if( !IsRootDir( Info.strPath ) )
		nStart++;

	// Make a File Name List out of the Entity Info List.
	VFS_FileNameMap Files;
	for( VFS_EntityInfoList::iterator iter = EntityInfos.begin(); iter != EntityInfos.end(); iter++ )
	{
		if( ( *iter ).eType == VFS_FILE )
			Files[ ( *iter ).strPath ] = ( *iter ).strPath.substr( nStart );
	}

	// Try to create an Archive from the File List.
	return VFS_Archive_CreateFromFileList( strArchiveFileName, Files, UsedFilters );
}

// Create an Archive from the specified File List.
VFS_BOOL VFS_Archive_CreateFromFileList( const VFS_String& strArchiveFileName, const VFS_FileNameMap& Files, const VFS_FilterNameList& UsedFilters )
{
	static VFS_BYTE Chunk[ FILE_COPY_CHUNK_SIZE ];

	// If there's already an Archive with the same File Name and it's open...
	VFS_EntityInfo Info;
	if( VFS_Archive_GetInfo( ToLower( strArchiveFileName ), Info ) )
	{
		// Check if the Archive is open.
		if( GetOpenArchives().find( ToLower( Info.strPath ) ) != GetOpenArchives().end() )
		{
			// Check if the Reference Count is != 0.
			if( GetOpenArchives()[ ToLower( Info.strPath ) ]->GetRefCount() > 0 )
			{
				// We don't want to manipulate an open Archive, do we?
				SetLastError( VFS_ERROR_IN_USE );
				return VFS_FALSE;
			}
			else
			{
				// Free the Archive.
				delete GetOpenArchives()[ ToLower( Info.strPath ) ];
				GetOpenArchives().erase( ToLower( Info.strPath ) );
			}
		}
	}
	else
	{
		Info.strPath = ToLower( strArchiveFileName );
		SetLastError( VFS_ERROR_NONE );
	}

	// Check the Filter Names and make a List of all Filter Pointers.
	VFS_FilterList Filters;
	for( VFS_FilterNameList::const_iterator iter = UsedFilters.begin(); iter != UsedFilters.end(); iter++ )
	{
		// Bad Filter Name?
		if( !VFS_ExistsFilter( *iter ) )
		{
			SetLastError( VFS_ERROR_INVALID_PARAMETER );
			return VFS_FALSE;
		}

		// Add the Filter.
		Filters.push_back( VFS_GetFilter( *iter ) );
	}

	// Check all Files.
	for( VFS_FileNameMap::const_iterator iter2 = Files.begin(); iter2 != Files.end(); iter2++ )
	{
		if( !VFS_File_Exists( ( *iter2 ).first ) )
		{
			SetLastError( VFS_ERROR_NOT_FOUND );
			return VFS_FALSE;
		}

		VFS_String strName;
		VFS_Util_GetName( ( *iter2 ).second, strName );
		if( strName.size() > VFS_MAX_NAME_LENGTH )
		{
			SetLastError( VFS_ERROR_INVALID_PARAMETER );
			return VFS_FALSE;
		}
	}

	// Make a list of the Directories to create.
	typedef vector< VFS_String > NameMap;
	NameMap Dirs;
	for( VFS_FileNameMap::const_iterator iter3 = Files.begin(); iter3 != Files.end(); iter3++ )
	{
		VFS_String strDir;
		VFS_Util_GetPath( ( *iter3 ).second, strDir );
		strDir = ToLower( strDir );
		if( strDir != VFS_TEXT( "" ) && find( Dirs.begin(), Dirs.end(), strDir ) == Dirs.end() )
		{
			// Add the top-level Dirs.
			while( strDir.rfind( VFS_PATH_SEPARATOR ) != VFS_String::npos )
			{
				if( find( Dirs.begin(), Dirs.end(), strDir ) != Dirs.end() )
					break;
				Dirs.push_back( ToLower( strDir ) );
				if( strDir.size() > VFS_MAX_NAME_LENGTH )
				{
					SetLastError( VFS_ERROR_INVALID_PARAMETER );
					return VFS_FALSE;
				}

				strDir = strDir.substr( 0, strDir.rfind( VFS_PATH_SEPARATOR ) );
			}

			if( find( Dirs.begin(), Dirs.end(), strDir ) == Dirs.end() )
			{
				Dirs.push_back( ToLower( strDir ) );
				if( strDir.size() > VFS_MAX_NAME_LENGTH )
				{
					SetLastError( VFS_ERROR_INVALID_PARAMETER );
					return VFS_FALSE;
				}
			}
		}
	}

	// (Re)create the Target File.
	VFS_Handle hFile = VFS_File_Create( Info.strPath + VFS_TEXT( "." ) + VFS_ARCHIVE_FILE_EXTENSION, VFS_READ | VFS_WRITE );
	if( hFile == VFS_INVALID_HANDLE_VALUE )
		return VFS_FALSE;

	// Write the Header.
	ARCHIVE_HEADER Header;
	memcpy( Header.ID, ARCHIVE_ID, sizeof( ARCHIVE_ID ) );
	Header.wVersion = VFS_VERSION;
	Header.dwNumFilters = ( VFS_DWORD )Filters.size();
	Header.dwNumDirs = ( VFS_DWORD )Dirs.size();
	Header.dwNumFiles = ( VFS_DWORD )Files.size();
	VFS_DWORD dwWritten;
	if( !VFS_File_Write( hFile, ( const VFS_BYTE* ) &Header, sizeof( ARCHIVE_HEADER ), &dwWritten ) )
	{
		VFS_File_Close( hFile );
		return VFS_FALSE;
	}

	// Write the Filters.
	for( VFS_FilterList::iterator iter4 = Filters.begin(); iter4 != Filters.end(); iter4++ )
	{
		ARCHIVE_FILTER Filter;
		strcpy( Filter.szName, ToLower( ( *iter4 )->GetName() ).c_str() );
		if( !VFS_File_Write( hFile, ( const VFS_BYTE* ) &Filter, sizeof( ARCHIVE_FILTER ) ) )
		{
			VFS_File_Close( hFile );
			return VFS_FALSE;
		}
	}

	// Write the Directories.
	for( NameMap::iterator iter5 = Dirs.begin(); iter5 != Dirs.end(); iter5++ )
	{
		ARCHIVE_DIR Dir;

		// Get the Name of the Dir and add it.
		VFS_String strName;
		VFS_Util_GetName( *iter5, strName );
		strcpy( Dir.szName, ToLower( strName ).c_str() );

		// Remove the <pathsep> and the Name from the path; the rest should be the Parent Directory.
		if( ( *iter5 ).find( VFS_PATH_SEPARATOR ) != VFS_String::npos )
		{
			// Get the Name of the Parent Directory.
			VFS_String strParentDir = ( *iter5 ).substr( 0, ( *iter5 ).rfind( VFS_PATH_SEPARATOR ) );

			// Get the Index of the Parent Directory.
			assert( find( Dirs.begin(), Dirs.end(), ToLower( strParentDir ) ) != Dirs.end() );
			Dir.dwParentIndex = ( VFS_DWORD )( find( Dirs.begin(), Dirs.end(), ToLower( strParentDir ) ) - Dirs.begin() );
		}
		else
			Dir.dwParentIndex = DIR_INDEX_ROOT;

		if( !VFS_File_Write( hFile, ( const VFS_BYTE* ) &Dir, sizeof( ARCHIVE_DIR ) ) )
		{
			VFS_File_Close( hFile );
			return VFS_FALSE;
		}
	}

	// Get the starting offset for the file data.
	VFS_DWORD dwOffset = sizeof( ARCHIVE_HEADER ) + Header.dwNumFilters * sizeof( ARCHIVE_FILTER ) +
		Header.dwNumDirs * sizeof( ARCHIVE_DIR ) + Header.dwNumFiles * sizeof( ARCHIVE_FILE );

	// Let the Filters store the configuration Data.
	for( VFS_FilterList::iterator iter6 = Filters.begin(); iter6 != Filters.end(); iter6++ )
	{
		// Setup diverse global Variables.
		g_ToBuffer.clear();

		// Call the Saver Proc.
		if( !( *iter6 )->SaveConfigData( Writer ) )
		{
			VFS_File_Close( hFile );
			return VFS_FALSE;
		}

		// Save it.
		VFS_DWORD dwPos = VFS_File_Tell( hFile );
		VFS_File_Seek( hFile, dwOffset, VFS_SET );
		VFS_File_Write( hFile, &*g_ToBuffer.begin(), ( VFS_DWORD )g_ToBuffer.size() );
		VFS_File_Seek( hFile, dwPos, VFS_SET );
		dwOffset += ( VFS_DWORD )g_ToBuffer.size();
	}

	// Write the Files.
	for( VFS_FileNameMap::const_iterator iter7 = Files.begin(); iter7 != Files.end(); iter7++ )
	{
		// Prepare the record.
		ARCHIVE_FILE File;

		// Get the Name of the File and add it.
		VFS_String strName;
		VFS_Util_GetName( ( *iter7 ).second, strName );
		strcpy( File.szName, ToLower( strName ).c_str() );

		// Get the Parent Dir ID.
		if( ( *iter7 ).second.find( VFS_PATH_SEPARATOR ) != VFS_String::npos )
		{
			// Get the Name of the Parent Directory.
			VFS_String strParentDir = ( *iter7 ).second.substr( 0, ( *iter7 ).second.rfind( VFS_PATH_SEPARATOR ) );

			// Get the Index of the Parent Directory.
			assert( find( Dirs.begin(), Dirs.end(), ToLower( strParentDir ) ) != Dirs.end() );
			File.dwDirIndex = ( VFS_DWORD )( find( Dirs.begin(), Dirs.end(), ToLower( strParentDir ) ) - Dirs.begin() );
		}
		else
			File.dwDirIndex = DIR_INDEX_ROOT;

		// Open the Source File.
		VFS_Handle hSrc = VFS_File_Open( ( *iter7 ).first, VFS_READ );
		if( hSrc == VFS_INVALID_HANDLE_VALUE )
		{
			VFS_File_Close( hFile );
			return VFS_FALSE;
		}

		// Store the uncompressed size.
		File.dwUncompressedSize = VFS_File_GetSize( hSrc );

		// Setup diverse global Variables.
		g_FromBuffer.clear();
		g_ToBuffer.clear();

		// Read in the File.
		VFS_DWORD dwRead;
		g_FromPos = 0;
		do
		{
			if( !VFS_File_Read( hSrc, Chunk, FILE_COPY_CHUNK_SIZE, &dwRead ) )
			{
				VFS_File_Close( hSrc );
				VFS_File_Close( hFile );
				return VFS_FALSE;
			}
			g_FromBuffer.reserve( g_FromBuffer.size() + dwRead );
			for( VFS_DWORD dwIndex = 0; dwIndex < dwRead; dwIndex++ )
				g_FromBuffer.push_back( Chunk[ dwIndex ] );
		}
		while( dwRead > 0 );

		// Close the File.
		VFS_File_Close( hSrc );

		// Call the Filters.
		VFS_EntityInfo Info;
		VFS_File_GetInfo( ( *iter7 ).first, Info );
		for( VFS_FilterList::iterator iter8 = Filters.begin(); iter8 != Filters.end(); iter8++ )
		{
			g_FromPos = 0;
			if( !( *iter8 )->Encode( Reader, Writer, Info ) )
			{
				VFS_ErrorCode eError = VFS_GetLastError();
				if( eError == VFS_ERROR_NONE )
					eError = VFS_ERROR_GENERIC;
				SetLastError( eError );
				VFS_File_Close( hFile );
				return VFS_FALSE;
			}
			g_FromBuffer = g_ToBuffer;
			g_ToBuffer.clear();
		}

		// Store the final Result.
		VFS_DWORD dwPos = VFS_File_Tell( hFile );
		VFS_File_Seek( hFile, dwOffset, VFS_SET );
		VFS_File_Write( hFile, &*g_FromBuffer.begin(), ( VFS_DWORD )g_FromBuffer.size() );
		File.dwCompressedSize = ( VFS_DWORD )g_FromBuffer.size();
		VFS_File_Seek( hFile, dwPos, VFS_SET );
		dwOffset += File.dwCompressedSize;

		if( !VFS_File_Write( hFile, ( const VFS_BYTE* ) &File, sizeof( ARCHIVE_FILE ) ) )
		{
			VFS_File_Close( hFile );
			return VFS_FALSE;
		}
	}

	// Close the File.
	if( !VFS_File_Close( hFile ) )
		return VFS_FALSE;

	return VFS_TRUE;
}

// Extract an Archive.
VFS_BOOL VFS_Archive_Extract( const VFS_String& strArchiveFileName, const VFS_String& strTargetDir )
{
	// Get Information about that Archive.
	VFS_EntityInfo Info;
	if( !VFS_Archive_GetInfo( strArchiveFileName, Info ) )
		return VFS_FALSE;

	// Check if the Archive isn't open yet.
	if( GetOpenArchives().find( ToLower( Info.strPath ) ) == GetOpenArchives().end() )
	{
		// Open the Archive.
		CArchive* pArchive = CArchive::Open( ToLower( Info.strPath ) );
		if( pArchive == NULL )
			return VFS_FALSE;

		// Add the Archive to the Open Archives List.
		GetOpenArchives()[ Info.strPath ] = pArchive;
	}

	// Extract the Archive to the Target Directory.
	return GetOpenArchives()[ ToLower( Info.strPath ) ]->Extract( strTargetDir );
}

// Extract a File.
VFS_BOOL VFS_Archive_ExtractFile( const VFS_String& strArchiveFileName, const VFS_String& strFile, const VFS_String& strTargetFile )
{
    // Create the Source File Name.
	VFS_String strFileName = WithoutTrailingSeparator( strArchiveFileName, VFS_TRUE ) + VFS_PATH_SEPARATOR + strFile;

	// Check if the File exists.
	VFS_EntityInfo Info;
	if( !VFS_File_GetInfo( strFileName, Info ) )
		return VFS_FALSE;

	// The file must be in an Archive.
	if( !Info.bArchived )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

    // Extract...
	return VFS_File_Copy( strFileName, strTargetFile );
}

// Information.

// Determines if an Archive with the specified file name exists.
VFS_BOOL VFS_Archive_Exists( const VFS_String& strArchiveFileName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	return CArchive::Exists( strArchiveFileName );
}

VFS_BOOL VFS_Archive_GetInfo( const VFS_String& strArchiveFileName, VFS_EntityInfo& Info )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Get Info and change Type.
	if( !VFS_File_GetInfo( strArchiveFileName + VFS_TEXT( "." ) + VFS_ARCHIVE_FILE_EXTENSION, Info ) )
		return VFS_FALSE;

	// Check the Extension.
	VFS_String strExtension;
	if( !VFS_Util_GetExtension( Info.strName, strExtension ) )
		return VFS_FALSE;
	if( ToLower( strExtension ) != ToLower( VFS_ARCHIVE_FILE_EXTENSION ) )
	{
		SetLastError( VFS_ERROR_NOT_AN_ARCHIVE );
		return VFS_FALSE;
	}

	// Remove the Extension (THE ONLY TIME THE USER IS CONFRONTATED WITH THE ARCHIVE FILE EXTENSION IS WHEN HE USES EXPLICITELY THE VFS_ARCHIVE_FILE_EXTENSION CONSTANT).
	Info.strName = Info.strName.substr( 0, Info.strName.size() - strExtension.size() - 1 );
	Info.strPath = Info.strPath.substr( 0, Info.strPath.size() - strExtension.size() - 1 );

	// Change the Type in the Entity Information Record.
	Info.eType = VFS_ARCHIVE;

	return VFS_TRUE;
}

VFS_BOOL VFS_Archive_GetUsedFilters( const VFS_String& strArchiveFileName, VFS_FilterNameList& FilterNames )
{
	// Get Information about that Archive.
	VFS_EntityInfo Info;
	if( !VFS_Archive_GetInfo( strArchiveFileName, Info ) )
		return VFS_FALSE;

	// Check if the Archive isn't open yet.
	if( GetOpenArchives().find( ToLower( Info.strPath ) ) == GetOpenArchives().end() )
	{
		// Open the Archive.
		CArchive* pArchive = CArchive::Open( ToLower( Info.strPath ) );
		if( pArchive == NULL )
			return VFS_FALSE;

		// Add the Archive to the Open Archives List.
		GetOpenArchives()[ Info.strPath ] = pArchive;
	}

	// Get the Filter Names List.
	FilterList Filters = GetOpenArchives()[ ToLower( Info.strPath ) ]->GetHeader()->Filters;
	FilterNames.clear();
	for( FilterList::const_iterator iter = Filters.begin(); iter != Filters.end(); iter++ )
	{
		FilterNames.push_back( ( *iter )->GetName() );
	}

	return VFS_TRUE;
}

// Archive Management.
VFS_BOOL VFS_Archive_Delete( const VFS_String& strArchiveFileName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	return VFS_File_Delete( strArchiveFileName + VFS_TEXT( "." ) + VFS_ARCHIVE_FILE_EXTENSION );
}

// Flush the Archive System.
VFS_BOOL VFS_Archive_Flush()
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	typedef vector< VFS_String > ArchiveList;
	ArchiveList ToClose;

	// For each open Archive.
	for( ArchiveMap::iterator iter = GetOpenArchives().begin(); iter != GetOpenArchives().end(); iter++ )
	{
		// If there are no more References to this Archive, then add it to the To Close List.
		if( ( *iter ).second->GetRefCount() == 0 )
		{
			ToClose.push_back( ( *iter ).first );
		}
	}

	// Close the Archives to Close.
	for( ArchiveList::iterator iter2 = ToClose.begin(); iter2 != ToClose.end(); iter2++ )
	{
		delete GetOpenArchives()[ ToLower( *iter2 ) ];
		GetOpenArchives().erase( ToLower( *iter2 ) );
	}

	return VFS_TRUE;
}

// Return a Map containing all Open Files.
ArchiveMap& GetOpenArchives()
{
	return g_OpenArchives;
}

//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
