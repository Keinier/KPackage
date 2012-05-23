//****************************************************************************
//**
//**    VFS_DIRS.CPP
//**    Directory Interface Implementation
//**
//**	Project:	VFS
//**	Component:	Dirs
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
//============================================================================
//    INTERFACE DATA
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================
static VFS_BOOL CreateRecursively( const VFS_String& strAbsoluteDirName );
static VFS_BOOL GetAbsoluteDirInfo( const VFS_String& strAbsoluteDirName, VFS_EntityInfo& Info );
static VFS_BOOL DeletionCallback( const VFS_EntityInfo& pInfo, void* pParam );
static VFS_BOOL ContentRetrievationCallback( const VFS_EntityInfo& pInfo, void* pParam );
static VFS_BOOL GetAllEntities( const VFS_String& strAbsoluteDirName, VFS_EntityInfoList& Files, VFS_EntityInfoList& Dirs );

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================
static VFS_BOOL CreateRecursively( const VFS_String& strAbsoluteDirName )
{
	// If we are in the root directory, then do just nothing :)
	if( IsRootDir( strAbsoluteDirName ) )
		return VFS_TRUE;

	// Get the parent segment.
	if( strAbsoluteDirName.rfind( VFS_PATH_SEPARATOR ) == VFS_String::npos )
	{
		SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}
	VFS_String strParent = strAbsoluteDirName.substr( 0, strAbsoluteDirName.rfind( VFS_PATH_SEPARATOR ) );

	// If strAbsoluteDirName is of the /bla/bla/bla kind.
	if( strParent == VFS_TEXT( "" ) )
		strParent = VFS_PATH_SEPARATOR;

	// Try to create the parent segment if it doesn't already exist.
	if( !VFS_Dir_Exists( strParent ) )
	{
		if( !CreateRecursively( strParent ) )
			return VFS_FALSE;
	}

    // Try to create the directory.
	if( !VFS_MKDIR( strAbsoluteDirName ) )
	{
		if( VFS_Dir_Exists( strAbsoluteDirName ) )
			SetLastError( VFS_ERROR_ALREADY_EXISTS );
		else
			SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}

	return VFS_TRUE;
}

static VFS_BOOL GetAbsoluteDirInfo( const VFS_String& strAbsoluteDirName, VFS_EntityInfo& Info )
{
	// Make a quick check first.
	if( ( VFS_EXISTS( strAbsoluteDirName ) && VFS_IS_DIR( strAbsoluteDirName ) ) )
	{
		// Fill out the Information.
		Info.bArchived = VFS_FALSE;
		Info.eType = VFS_DIR;
		Info.lSize = 0;
		Info.strPath = strAbsoluteDirName;
		VFS_Util_GetName( Info.strPath, Info.strName );
		return VFS_Util_GetName( Info.strPath, Info.strName );
	}
	if( CArchive::Exists( strAbsoluteDirName ) )
	{
		// Fill out the Information.
		Info.bArchived = VFS_TRUE;
		Info.eType = VFS_DIR;
		Info.lSize = 0;
		Info.strPath = strAbsoluteDirName;
		VFS_Util_GetName( Info.strPath, Info.strName );
		return VFS_Util_GetName( Info.strPath, Info.strName );
	}

	// Tokenize the Dir Name in pairs of (before Path Separator) => (after Path Separator).
	typedef std::map< VFS_String, VFS_String > PairMap;
	PairMap Pairs;
	VFS_String::size_type nPos = 0; // Yup, ignore the / if it's at the beginning.
	while( ( nPos = strAbsoluteDirName.find( VFS_PATH_SEPARATOR, nPos + 1 ) ) != VFS_String::npos )
	{
		VFS_String strFirst, strSecond;
		strFirst = strAbsoluteDirName.substr( 0, nPos );
		strSecond = strAbsoluteDirName.substr( nPos + 1 );
		Pairs[ strFirst ] = strSecond;
	}

	// For each Entry...
	for( PairMap::iterator iter = Pairs.begin(); iter != Pairs.end(); iter++ )
	{
		// Exists the Directory?
		if( VFS_EXISTS( ( *iter ).first ) && VFS_IS_DIR( ( *iter ).first ) )
			continue;

		// Exists an Archive?
		if( CArchive::Exists( ( *iter ).first ) )
		{
			// Open the Archive...
			if( GetOpenArchives().find( ToLower( ( *iter ).first ) ) == GetOpenArchives().end() )
			{
				// Open the Archive manually.
				CArchive* pArchive = CArchive::Open( ( *iter ).first );
				if( pArchive == NULL )
					return VFS_FALSE;

				// Add it to the Open Archives Map.
				GetOpenArchives()[ ToLower( ( *iter ).first ) ] = pArchive;
			}

			// Is there a Subdirectory with the path saved in the second part of the Pair?
			if( GetOpenArchives()[ ToLower( ( *iter ).first ) ]->ContainsDir( ( *iter ).second ) )
			{
				// Fill out the Information.
				Info.bArchived = VFS_TRUE;
				Info.eType = VFS_DIR;
				Info.lSize = 0;
				Info.strPath = strAbsoluteDirName;
				VFS_Util_GetName( Info.strPath, Info.strName );
				return VFS_Util_GetName( Info.strPath, Info.strName );
			}
		}

		return VFS_FALSE;
	}

	return VFS_FALSE; // Should never reach here...
}

static VFS_BOOL DeletionCallback( const VFS_EntityInfo& Info, void* pParam )
{
	// Archive?
	if( Info.eType == VFS_ARCHIVE )
	{
		if( !VFS_Archive_Delete( Info.strPath ) )
			return VFS_FALSE;
	}
	// Dir?
	else if( Info.eType == VFS_DIR )
	{
		VFS_EntityInfoList* pDirs = ( VFS_EntityInfoList* ) pParam;
		pDirs->push_back( Info );
	}
	// Standard File?
	else if( Info.eType == VFS_FILE && !Info.bArchived )
	{
		if( !VFS_File_Delete( Info.strPath ) )
			return VFS_FALSE;
	}

	return VFS_TRUE;
}

static VFS_BOOL ContentRetrievationCallback( const VFS_EntityInfo& Info, void* pParam )
{
	VFS_EntityInfoList* pList = ( VFS_EntityInfoList* ) pParam;
	pList->push_back( Info );
	return VFS_TRUE;
}

static VFS_BOOL GetAllEntities( const VFS_String& strAbsoluteDirName, VFS_EntityInfoList& Files, VFS_EntityInfoList& Dirs )
{
	// Clear Everything.
	Files.clear();
	Dirs.clear();

    // Try to find the First File.
	VFS_String strName;
	VFS_BOOL bIsDir;
	VFS_LONG lSize;
	if( !VFS_FIND_FILE( WithoutTrailingSeparator( strAbsoluteDirName, VFS_TRUE ) + VFS_PATH_SEPARATOR + VFS_TEXT( "*" ), strName, bIsDir, lSize, 0 ) )
		return VFS_TRUE;

	// Find more files.
	do
	{
		if( strName == VFS_TEXT( "." ) || strName == VFS_TEXT( ".." ) )
			continue;

		VFS_EntityInfo Info;
		Info.bArchived = VFS_FALSE;
		Info.eType = bIsDir ? VFS_DIR : VFS_FILE;
		Info.lSize = lSize;
		Info.strName = strName;
		Info.strPath = WithoutTrailingSeparator( strAbsoluteDirName, VFS_TRUE ) + VFS_PATH_SEPARATOR + strName;
		if( bIsDir )
		{
			VFS_String strExtension;
			VFS_Util_GetExtension( Info.strName, strExtension );
			if( ToLower( strExtension ) == ToLower( VFS_ARCHIVE_FILE_EXTENSION ) )
				Info.eType = VFS_ARCHIVE;
			Dirs.push_back( Info );
		}
		else
		{
			Files.push_back( Info );
		}
	}
	while( VFS_FIND_FILE( VFS_TEXT( "wedontcare" ), strName, bIsDir, lSize, 1 ) );

	// End the Search.
	return VFS_FIND_FILE( VFS_TEXT( "wedontcare" ), strName, bIsDir, lSize, 2 );
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================
// Directory Management.

// Create a new Directory in the first root path.
VFS_BOOL VFS_Dir_Create( const VFS_String& strDirName, VFS_BOOL bRecursive )	// Recursive mode would create a directory c:\alpha\beta even if alpha doesn't exist.
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// The Directory already exists?
	if( VFS_Dir_Exists( strDirName ) )
	{
		SetLastError( VFS_ERROR_ALREADY_EXISTS );
		return VFS_FALSE;
	}

	// No Root Paths specified yet?
	if( GetRootPaths().size() == 0 && !VFS_Util_IsAbsoluteFileName( strDirName ) )
	{
		SetLastError( VFS_ERROR_NO_ROOT_PATHS_DEFINED );
		return VFS_FALSE;
	}

	// Make it absolute...
	VFS_String strAbsoluteDirName = WithoutTrailingSeparator( strDirName, VFS_FALSE );
	if( !VFS_Util_IsAbsoluteFileName( strAbsoluteDirName ) )
		strAbsoluteDirName = WithoutTrailingSeparator( GetRootPaths()[ 0 ], VFS_TRUE ) + VFS_PATH_SEPARATOR + strDirName;

    // Should we use a recursive function?
	if( bRecursive )
		return CreateRecursively( strAbsoluteDirName );

    // Try to create the directory.
	if( !VFS_MKDIR( strAbsoluteDirName ) )
	{
		if( VFS_Dir_Exists( strAbsoluteDirName ) )
			SetLastError( VFS_ERROR_ALREADY_EXISTS );
		else
			SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}

	return VFS_TRUE;
}

// Delete the Directory with the specified Name.
VFS_BOOL VFS_Dir_Delete( const VFS_String& strDirName, VFS_BOOL bRecursive )	// Recursive mode would delete a directory c:\alpha even if it contains files and/or subdirectories.
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	VFS_EntityInfo Info;
	if( !VFS_Dir_GetInfo( strDirName, Info ) )
		return VFS_FALSE;

	// If the Directory resides in an Archive, fail.
	if( Info.bArchived )
	{
		SetLastError( VFS_ERROR_CANT_MANIPULATE_ARCHIVES );
		return VFS_FALSE;
	}

	// Recursive Mode?
	if( bRecursive )
	{
		// Iterate through the Directory and delete everything.
		VFS_EntityInfoList Dirs;
		if( !VFS_Dir_Iterate( strDirName, DeletionCallback, VFS_TRUE, ( void* ) &Dirs ) )
			return VFS_FALSE;

		// Check if an error occured.
		VFS_ErrorCode eError = VFS_GetLastError();
		if( eError != VFS_ERROR_NONE )
		{
			SetLastError( eError );
			return VFS_FALSE;
		}

		// Delete all Dirs (in reverse order).
		for( VFS_EntityInfoList::reverse_iterator iter = Dirs.rbegin(); iter != Dirs.rend(); iter++ )
		{
			if( !VFS_Dir_Delete( ( *iter ).strPath, VFS_FALSE ) )
				return VFS_FALSE;
		}
	}

	if( !VFS_RMDIR( Info.strPath ) )
	{
		SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}
	
	return VFS_TRUE;
}

// Information.
VFS_BOOL VFS_Dir_Exists( const VFS_String& strDirName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

    // Try to get Dir Infos.
	VFS_EntityInfo Info;
	if( !VFS_Dir_GetInfo( strDirName, Info ) )
	{
		SetLastError( VFS_ERROR_NONE );
		return VFS_FALSE;
	}

	return VFS_TRUE;
}

VFS_BOOL VFS_Dir_GetInfo( const VFS_String& strDirName, VFS_EntityInfo& Info )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// No Root Paths specified yet?
	if( GetRootPaths().size() == 0 && !VFS_Util_IsAbsoluteFileName( strDirName ) )
	{
		SetLastError( VFS_ERROR_NO_ROOT_PATHS_DEFINED );
		return VFS_FALSE;
	}

	// Absolute?
	if( VFS_Util_IsAbsoluteFileName( strDirName ) )
	{
		if( GetAbsoluteDirInfo( strDirName, Info ) )
			return VFS_TRUE;

		SetLastError( VFS_ERROR_NOT_FOUND );
		return VFS_FALSE;
	}

	// Test in each Root Path...
	for( VFS_RootPathList::iterator iter = GetRootPaths().begin(); iter != GetRootPaths().end(); iter++ )
	{
		VFS_String strAbsoluteDirName = WithoutTrailingSeparator( *iter, VFS_TRUE ) + VFS_PATH_SEPARATOR + strDirName;
		if( GetAbsoluteDirInfo( strAbsoluteDirName, Info ) )
			return VFS_TRUE;
	}

	SetLastError( VFS_ERROR_NOT_FOUND );
	return VFS_FALSE;
}

// Iterate a Directory and call the iteration procedure for each 
VFS_BOOL VFS_Dir_Iterate( const VFS_String& strDirName, VFS_DirIterationProc pIterationProc, VFS_BOOL bRecursive, void* pParam )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Get Dir Information.
	VFS_EntityInfo Info;
	if( !VFS_Dir_GetInfo( strDirName, Info ) )
		return VFS_FALSE;

	// If the Dir is in an Archive.
	if( Info.bArchived )
	{
		// For each Substring [0..PATH_SEPARATOR_POS]...
		VFS_String strTemp = WithoutTrailingSeparator( Info.strPath, VFS_TRUE ) + VFS_PATH_SEPARATOR;
		VFS_String::size_type szPos = strTemp.size();
		while( ( szPos = strTemp.rfind( VFS_PATH_SEPARATOR, szPos - 1 ) ) != VFS_String::npos )
		{
			VFS_String strArchive = ToLower( strTemp.substr( 0, szPos ) );

			// Is this an Archive?
			if( CArchive::Exists( strArchive ) )
			{
				// Open the Archive...
				if( GetOpenArchives().find( strArchive ) == GetOpenArchives().end() )
				{
					// Open the Archive manually.
					CArchive* pArchive = CArchive::Open( strArchive );
					if( pArchive == NULL )
						return VFS_FALSE;

					// Add it to the Open Archives Map.
					GetOpenArchives()[ strArchive ] = pArchive;
				}

				return GetOpenArchives()[ strArchive ]->IterateDir( strTemp.substr( szPos + 1 ), pIterationProc, bRecursive, pParam );
			}
		}

		SetLastError( VFS_ERROR_NOT_FOUND );
		return VFS_FALSE;
	}

    // Get the Dir's Contents.
	VFS_EntityInfoList Files, Dirs;
	if( !GetAllEntities( Info.strPath, Files, Dirs ) )
	{
		SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}

	// Iterate through all Subdirs...
	for( VFS_EntityInfoList::iterator iter = Dirs.begin(); iter != Dirs.end(); iter++ )
	{
		// Iterate.
        if( !pIterationProc( *iter, pParam ) )
			return VFS_TRUE;

		// Recurse?
		if( bRecursive )
			VFS_Dir_Iterate( ( *iter ).strPath, pIterationProc, bRecursive, pParam );
	}

	// Iterate through all Files...
	for( VFS_EntityInfoList::iterator iter2 = Files.begin(); iter2 != Files.end(); iter2++ )
	{
		// Iterate.
		if( !pIterationProc( *iter2, pParam ) )
			return VFS_TRUE;
	}

	return VFS_TRUE;
}

// Get the Contents of a Directory.
VFS_BOOL VFS_Dir_GetContents( const VFS_String& strDirName, VFS_EntityInfoList& EntityInfoList, VFS_BOOL bRecursive )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	return VFS_Dir_Iterate( strDirName, ContentRetrievationCallback, bRecursive, ( void* ) &EntityInfoList );
}

//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
