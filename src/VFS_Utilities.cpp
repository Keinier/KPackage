//****************************************************************************
//**
//**    VFS_UTILITIES.CPP
//**    No Description
//**
//**	Project:	VFS
//**	Component:	Utilities
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
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================
// Returns the Path for a File Name.
VFS_BOOL VFS_Util_GetPath( const VFS_String& strFileName, VFS_String& strPath )
{
	// Following exceptions:
	// Path of "/" is ""
	// Path of (char)":/" is (""
	if( ( strFileName.size() == 1 && strFileName[ 0 ] == VFS_PATH_SEPARATOR ) ||
		( strFileName.size() == 3 && strFileName[ 1 ] == VFS_TEXT( ':' ) && strFileName[ 2 ] == VFS_PATH_SEPARATOR ) )
	{
		strPath = VFS_TEXT( "" );
		return VFS_TRUE;
	}
	strPath = WithoutTrailingSeparator( strFileName, VFS_FALSE );

	// Is there a trailing backslash?
	if( strPath.rfind( VFS_PATH_SEPARATOR ) != VFS_String::npos )
	{
		// Remove all the text starting from the trailing backslash.
		strPath = strPath.substr( 0, strPath.rfind( VFS_PATH_SEPARATOR ) );
	}
	else
	{
		strPath = VFS_TEXT( "" );
	}

	return VFS_TRUE;
}

// Returns the Name for a File Name.
VFS_BOOL VFS_Util_GetName( const VFS_String& strFileName, VFS_String& strName )
{
	// Following exceptions:
	// Name of "/" is "/"
	// Name of (char)":/" is (char)":/"
	if( ( strFileName.size() == 1 && strFileName[ 0 ] == VFS_PATH_SEPARATOR ) ||
		( strFileName.size() == 3 && strFileName[ 1 ] == VFS_TEXT( ':' ) && strFileName[ 2 ] == VFS_PATH_SEPARATOR ) )
	{
		strName = strFileName;
		return VFS_TRUE;
	}

	strName = WithoutTrailingSeparator( strFileName, VFS_TRUE );

	// Is there a trailing backslash?
	if( strName.rfind( VFS_PATH_SEPARATOR ) != VFS_String::npos )
	{
		// Remove all the text from the beginning to the trailing backslash.
		strName = strName.substr( strName.rfind( VFS_PATH_SEPARATOR ) + 1 );
	}

	return VFS_TRUE;
}

// Returns the Base Name for a File Name.
VFS_BOOL VFS_Util_GetBaseName( const VFS_String& strFileName, VFS_String& strBaseName )
{
	// Get the Name.
	if( !VFS_Util_GetName( strFileName, strBaseName ) )
		return VFS_FALSE;

	// Is there a point in the string
	if( strBaseName.rfind( VFS_TEXT( '.' ) ) != VFS_String::npos )
	{
		// Remove the Extension.
		strBaseName = strBaseName.substr( 0, strBaseName.rfind( VFS_TEXT( '.' ) ) );
	}

	return VFS_TRUE;
}

// Returns the Extension for a File Name.
VFS_BOOL VFS_Util_GetExtension( const VFS_String& strFileName, VFS_String& strExtension )
{
	// Get the Name.
	if( !VFS_Util_GetName( strFileName, strExtension ) )
		return VFS_FALSE;

	// Is there a point in the string
	if( strExtension.rfind( VFS_TEXT( '.' ) ) != VFS_String::npos )
	{
		// Remove the Extension.
		strExtension = strExtension.substr( strExtension.rfind( VFS_TEXT( '.' ) ) + 1 );
	}
	else
	{
		strExtension = VFS_TEXT( "" );
	}

	return VFS_TRUE;
}

// Determines whether the specified file name is absolute.
VFS_BOOL VFS_Util_IsAbsoluteFileName( const VFS_String& strFileName )
{
	// There are two possibilities for an absolute File Name.
	// - <path separator>.......
	// - <drive letter><path separator>.......
	if( strFileName.size() == 0 )
		return VFS_FALSE;

	if( strFileName[ 0 ] == VFS_PATH_SEPARATOR )
		return VFS_TRUE;

	if( strFileName.size() < 3 )
		return VFS_FALSE;

	if( isalpha( strFileName[ 0 ] ) && strFileName[ 1 ] == VFS_TEXT( ':' ) && strFileName[ 2 ] == VFS_PATH_SEPARATOR )
		return VFS_TRUE;

	return VFS_FALSE;
}

//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
