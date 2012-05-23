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
static VFS_BOOL g_bInit = VFS_FALSE;
static VFS_ErrorCode g_eLastError = VFS_ERROR_NONE;
static VFS_PCSTR g_ErrorStrings[] =
{
	VFS_TEXT( "No Error (VFS_ERROR_NONE)" ),
	VFS_TEXT( "VFS not initialized yet (VFS_ERROR_NOT_INITIALIZED_YET)" ),
	VFS_TEXT( "VFS already initialized (VFS_ERROR_ALREADY_INITIALIZED)" ),
	VFS_TEXT( "Entity already exists (VFS_ERROR_ALREADY_EXISTS)" ),
	VFS_TEXT( "Entity not found (VFS_ERROR_NOT_FOUND)" ),
	VFS_TEXT( "Invalid Parameter (VFS_ERROR_INVALID_PARAMETER)" ),
	VFS_TEXT( "Generic Error (VFS_ERROR_GENERIC)" ),
	VFS_TEXT( "Invalid Error Code (VFS_ERROR_INVALID_ERROR_CODE)" ),
	VFS_TEXT( "No Root Paths defined yet (VFS_ERROR_NO_ROOT_PATHS_DEFINED)" ),
	VFS_TEXT( "Permission denied (VFS_ERROR_PERMISSION_DENIED)" ),
	VFS_TEXT( "Entity in use (VFS_ERROR_IN_USE)" ),
	VFS_TEXT( "Can't manipulate Archives (VFS_ERROR_CANT_MANIPULATE_ARCHIVES)" ),
	VFS_TEXT( "Not an Archive (VFS_ERROR_NOT_AN_ARCHIVE)" ),
	VFS_TEXT( "Missing Filters (VFS_ERROR_MISSING_FILTERS)" ),
	VFS_TEXT( "Invalid Archive Format (VFS_ERROR_INVALID_ARCHIVE_FORMAT)" )
};
static FilterMap g_Filters;
static VFS_RootPathList g_RootPaths;

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
// Initializes the VFS.
VFS_BOOL VFS_Init()
{
	// Already initialized.
	if( IsInit() )
	{
		SetLastError( VFS_ERROR_ALREADY_INITIALIZED );
		return VFS_FALSE;
	}

	// Clear the Filters List.
	g_Filters.clear();

	// Clear the Open Archive and File Maps.
	GetOpenArchives().clear();
	GetOpenFiles().clear();

	// Toggle the Initialized Flag.
	g_bInit = VFS_TRUE;

	return VFS_TRUE;
}

// Uninitializes the VFS.
VFS_BOOL VFS_Shutdown()
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Flush all the stuff.
	VFS_Flush();

#ifdef VFS_DEBUG
	static VFS_CHAR szBuffer[ 1024 ];

	// Release the Files.
	if( GetOpenFiles().size() > 0 )
	{
		// Print a Warning Message.
		swprintf( szBuffer, VFS_TEXT( "VFS_Shutdown(): %u Files haven't been released:" ), GetOpenFiles().size() );
		wprintf( szBuffer );

        // Release all Files.
		for( FileMap::iterator iter = GetOpenFiles().begin(); iter != GetOpenFiles().end(); iter++ )
		{
			VFS_UINT uRefCount = ( *iter ).second->GetRefCount();

			// Print a Warning Message for this File.
			swprintf( szBuffer, VFS_TEXT( "VFS_Shutdown():   %s (%u References)" ), ( *iter ).second->GetFileName().c_str(), uRefCount );

			// Release the File.
			while( uRefCount-- > 0 )
				( *iter ).second->Release();
		}
	}

	// Release the Archives.
	if( GetOpenArchives().size() > 0 )
	{
		// Print a Warning Message.
		swprintf( szBuffer, VFS_TEXT( "VFS_Shutdown(): %u Archives haven't been released:" ), GetOpenArchives().size() );
		wprintf( szBuffer );

        // Release all Files.
		for( ArchiveMap::iterator iter = GetOpenArchives().begin(); iter != GetOpenArchives().end(); iter++ )
		{
			VFS_UINT uRefCount = ( *iter ).second->GetRefCount();

			// Print a Warning Message for this File.
			swprintf( szBuffer, VFS_TEXT( "VFS_Shutdown():   %s (%u References)" ), ( *iter ).second->GetFileName().c_str(), uRefCount );

			delete ( *iter ).second;
		}
	}

	// You don't have to unregister the filters.
#endif

	// Toggle the Initialized Flag.
	g_bInit = VFS_FALSE;

	return VFS_TRUE;
}

// Returns whether the VFS is initialized.
VFS_BOOL VFS_IsInit()
{
	return IsInit();
}

// Registers a Filter.
VFS_BOOL VFS_RegisterFilter( VFS_Filter* pFilter )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Parameter?
	if( !pFilter || VFS_String( pFilter->GetName() ).size() > VFS_MAX_NAME_LENGTH )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Already in List?
	if( g_Filters.find( ToLower( pFilter->GetName() ) ) != g_Filters.end() )
	{
		SetLastError( VFS_ERROR_ALREADY_EXISTS );
		return VFS_FALSE;
	}

	// Add.
	g_Filters[ ToLower( pFilter->GetName() ) ] = pFilter;

	return VFS_TRUE;
}

// Unregisters a Filter.
VFS_BOOL VFS_UnregisterFilter( VFS_Filter* pFilter )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Parameter?
	if( !pFilter )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Not in List yet?
	if( g_Filters.find( ToLower( pFilter->GetName() ) ) == g_Filters.end() )
	{
		SetLastError( VFS_ERROR_ALREADY_EXISTS );
		return VFS_FALSE;
	}

	// Remove.
	g_Filters.erase( pFilter->GetName() );

	return VFS_TRUE;
}

// Unregisters a Filter.
VFS_BOOL VFS_UnregisterFilter( VFS_DWORD dwIndex )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Parameter?
	if( dwIndex >= g_Filters.size() )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the Iterator to the first Element.
	map< VFS_String, VFS_Filter* >::iterator iter = g_Filters.begin();

	// Jump to the specified Index.
	while( dwIndex-- > 0 )
		iter++;

	// Remove the Filter.
	g_Filters.erase( iter );

	return VFS_TRUE;
}

// Unregisters a Filter.
VFS_BOOL VFS_UnregisterFilter( const VFS_String& strFilterName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Parameter?
	if( g_Filters.find( ToLower( strFilterName ) ) == g_Filters.end() )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Remove.
	g_Filters.erase( strFilterName );

	return VFS_TRUE;
}

// Returns whether a Filter exists.
VFS_BOOL VFS_ExistsFilter( const VFS_String& strFilterName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	return g_Filters.find( ToLower( strFilterName ) ) != g_Filters.end();
}

// Returns the Filter for the specified Name.
const VFS_Filter* VFS_GetFilter( const VFS_String& strFilterName )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_INVALID_POINTER_VALUE;
	}

	// Invalid Parameter?
	if( g_Filters.find( ToLower( strFilterName ) ) == g_Filters.end() )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_INVALID_POINTER_VALUE;
	}

	return g_Filters[ ToLower( strFilterName ) ];
}

// Returns the Number of registered Filters.
VFS_DWORD VFS_GetNumFilters()
{
	return ( VFS_DWORD )g_Filters.size();
}

// Returns the Filter for the specified Index.
const VFS_Filter* VFS_GetFilter( VFS_DWORD dwIndex )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Parameter?
	if( dwIndex >= g_Filters.size() )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the Iterator to the first Element.
	map< VFS_String, VFS_Filter* >::iterator iter = g_Filters.begin();

	// Jump to the specified Index.
	while( dwIndex-- > 0 )
		iter++;

	// Remove the Filter.
	return iter->second;
}

// Returns all Filters.
VFS_BOOL VFS_GetFilters( VFS_FilterList& Filters )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}
	
	// Empty the filter list.
	Filters.clear();

	// For each filter...
	for( FilterMap::const_iterator iter = g_Filters.begin(); iter != g_Filters.end(); iter++ )
	{
		// Add the filter.
		Filters.push_back( iter->second );
	}

	return VFS_TRUE;
}

// Returns all Filter Names.
VFS_BOOL VFS_GetFilterNames( VFS_FilterNameList& FilterNames )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}
	
	// Empty the filter names list.
	FilterNames.clear();

	// For each filter...
	for( FilterMap::const_iterator iter = g_Filters.begin(); iter != g_Filters.end(); iter++ )
	{
		// Add the filter name.
		FilterNames.push_back( iter->first );
	}

	return VFS_TRUE;
}

// Adds a Root Path.
VFS_BOOL VFS_AddRootPath( const VFS_String& strRootPath )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Not an absolute path?
	if( !VFS_Util_IsAbsoluteFileName( strRootPath ) )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Already in the list?
	if( find( g_RootPaths.begin(), g_RootPaths.end(), ToLower( WithoutTrailingSeparator( strRootPath, VFS_FALSE ) ) ) != g_RootPaths.end() )
	{
		SetLastError( VFS_ERROR_ALREADY_EXISTS );
		return VFS_FALSE;
	}

	// Add the Root Path.
	g_RootPaths.push_back( ToLower( WithoutTrailingSeparator( strRootPath, VFS_FALSE ) ) );

	return VFS_TRUE;
}

// Removes a Root Path.
VFS_BOOL VFS_RemoveRootPath( const VFS_String& strRootPath )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Not in the list yet?
	if( find( g_RootPaths.begin(), g_RootPaths.end(), ToLower( WithoutTrailingSeparator( strRootPath, VFS_FALSE ) ) ) == g_RootPaths.end() )
	{
		SetLastError( VFS_ERROR_NOT_FOUND );
		return VFS_FALSE;
	}

	// Remove the Root Path.
	g_RootPaths.erase( find( g_RootPaths.begin(), g_RootPaths.end(), ToLower( WithoutTrailingSeparator( strRootPath, VFS_FALSE ) ) ) );

	return VFS_TRUE;
}

// Removes a Root Path.
VFS_BOOL VFS_RemoveRootPath( VFS_DWORD dwIndex )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid parameter?
	if( dwIndex >= g_RootPaths.size() )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Remove the Root Path.
	g_RootPaths.erase( g_RootPaths.begin() + dwIndex );

	return VFS_TRUE;
}

// Returns the Number of Root Paths.
VFS_DWORD VFS_GetNumRootPaths()
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	return ( VFS_DWORD )g_RootPaths.size();
}

// Returns the Root Path for the specified Index.
VFS_BOOL VFS_GetRootPath( VFS_DWORD dwIndex, VFS_String& strRootPath )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Invalid Parameter?
	if( dwIndex >= g_RootPaths.size() )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Get the Root Path.
	strRootPath = g_RootPaths[ dwIndex ];

	return VFS_TRUE;
}

// Returns all Root Paths.
VFS_BOOL VFS_GetRootPaths( VFS_RootPathList& RootPaths )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Get the Root Paths.
	RootPaths = g_RootPaths;

	return VFS_TRUE;
}

// Flushes the VFS.
VFS_BOOL VFS_Flush()
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Close all open Files which RefCount == 0.
	typedef vector< VFS_String > FileList;
	FileList ToDelete;
	for( FileMap::iterator iter = GetOpenFiles().begin(); iter != GetOpenFiles().end(); iter++ )
	{
		if( ( *iter ).second->GetRefCount() == 0 )
		{
			ToDelete.push_back( ( *iter ).first );
		}
	}
	for( FileList::iterator iter2 = ToDelete.begin(); iter2 != ToDelete.end(); iter2++ )
	{
		GetOpenFiles()[ *iter2 ]->Release();
		GetOpenFiles().erase( *iter2 );
	}

	// Flush the Archive System.
	VFS_Archive_Flush();

	return VFS_TRUE;
}

// Returns if an Entitiy (File, Directory, Archive) with the specified Name exists.
// The Search Order is:
// - Archive
// - Directory
// - File
VFS_BOOL VFS_ExistsEntity( const VFS_String& strPath )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Is this a Directory?
	if( VFS_Dir_Exists( strPath ) )
		return VFS_TRUE;

	// Is this a File?
	if( VFS_File_Exists( strPath ) )
		return VFS_TRUE;

	return VFS_FALSE;
}

// Returns Information about an Entitiy (File, Directory, Archive) with the specified Name.
// The Search Order is:
// - Archive
// - Directory
// - File
VFS_BOOL VFS_GetEntityInfo( const VFS_String& strPath, VFS_EntityInfo& Info )
{
	// Not initialized yet?
	if( !IsInit() )
	{
		SetLastError( VFS_ERROR_NOT_INITIALIZED_YET );
		return VFS_FALSE;
	}

	// Is this a Directory?
	if( VFS_Dir_Exists( strPath ) )
		return VFS_Dir_GetInfo( strPath, Info );

	// Is this a File?
	if( VFS_File_Exists( strPath ) )
		return VFS_File_GetInfo( strPath, Info );

	// Such an entity doesn't exist.
	SetLastError( VFS_ERROR_NOT_FOUND );

	return VFS_FALSE;
}

// Returns the current VFS Version.
VFS_WORD VFS_GetVersion()
{
	return VFS_VERSION;
}

// Returns the last Error Code.
// You may call this function even you didn't initialize
VFS_ErrorCode VFS_GetLastError()
{
	// Reset the Error Code.
	VFS_ErrorCode eError = g_eLastError;
	g_eLastError = VFS_ERROR_NONE;

	return eError;
}

// Returns a String describing the specified Error Code.
// You may call this function even you didn't initialize the VFS before. If the
// error code passed to this function is invalid, the function will return the
// string associated with the error code VFS_ERROR_INVALID_ERROR_CODE.
VFS_PCSTR VFS_GetErrorString( VFS_ErrorCode eError )
{
	if( eError >= VFS_NUM_ERRORS )
	{
		return g_ErrorStrings[ VFS_ERROR_INVALID_ERROR_CODE ];
	}

	return g_ErrorStrings[ eError ];
}

// Internal Stuff.

// Is the VFS initialized?
const VFS_BOOL& IsInit()
{
	return g_bInit;
}

// Sets the Last Error Code (Implementation only).
void SetLastError( VFS_ErrorCode eError )
{
	g_eLastError = eError;
}

// Get the Root Path List.
VFS_RootPathList& GetRootPaths()
{
	return g_RootPaths;
}

// (Always) removes a trailing Path Separator Char.
VFS_String WithoutTrailingSeparator( const VFS_String& strFileName, VFS_BOOL bForce )
{
	VFS_String strResult = strFileName;

	// If we are in the root directory, then do just nothing :)
	if( strFileName.size() == 0 || ( !bForce && IsRootDir( strFileName ) ) )
		return strResult;

	if( *( strResult.end() -1 ) == VFS_PATH_SEPARATOR )
		return strResult.substr( 0, strResult.size() - 1 );

	return strResult;
}

// Determines if the specified File Name represents a Root Directory.
VFS_BOOL IsRootDir( const VFS_String& strFileName )
{
	if( strFileName.size() == 0 )
		return VFS_FALSE;

	if( strFileName.size() == 1 && strFileName[ 1 ] == VFS_PATH_SEPARATOR )
		return VFS_TRUE;

	if( strFileName.size() == 3 && isalpha( strFileName[ 0 ] ) && strFileName[ 1 ] == VFS_TEXT( ':' ) && strFileName[ 2 ] == VFS_PATH_SEPARATOR )
		return VFS_TRUE;

	return VFS_FALSE;
}

// Makes a String lower-cased.
VFS_String ToLower( const VFS_String& strString )
{
	VFS_String strResult = strString;
	for( VFS_String::iterator iter = strResult.begin(); iter != strResult.end(); iter++ )
		*iter = tolower( *iter );
	return strResult;
}

//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
