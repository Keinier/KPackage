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
//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
// --- StdIO File Class ---

// Constructor / Destructor.
CStdIOFile::CStdIOFile( const VFS_String& strAbsoluteFileName, VFS_BOOL bOpen )
: IFile( strAbsoluteFileName )
{
	m_bReadOnly = VFS_FALSE;

	// Create?
	if( !bOpen )
		m_pFile = VFS_FOPEN( strAbsoluteFileName, VFS_String( VFS_TEXT( "w+b" ) ) );
	else
	{
		// Try to open for r/w access.
		m_pFile = VFS_FOPEN( strAbsoluteFileName, VFS_String( VFS_TEXT( "r+b" ) ) );

		// If this fails, try to open for read-only access.
		if( m_pFile == NULL )
		{
			m_pFile = VFS_FOPEN( strAbsoluteFileName, VFS_String( VFS_TEXT( "rb" ) ) );
			m_bReadOnly = VFS_TRUE;
		}
	}

	// Error occured?
	if( !m_pFile )
		SetLastError( VFS_ERROR_NOT_FOUND );
}

CStdIOFile::~CStdIOFile()
{
	if( m_pFile )
		fclose( m_pFile );
}

// Is the File valid?
VFS_BOOL CStdIOFile::IsValid() const
{
	return m_pFile != NULL;
}

// Read / Write.
VFS_BOOL CStdIOFile::Read( VFS_BYTE* pBuffer, VFS_DWORD dwToRead, VFS_DWORD* pRead )
{
	// Invalid File?
	if( m_pFile == NULL )
	{
		SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}

	// Invalid Buffer Pointer?
	if( dwToRead > 0 && pBuffer == NULL )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Read.
	size_t szRead;
	if( dwToRead > 0 )
		szRead = fread( pBuffer, sizeof( VFS_BYTE ), dwToRead, m_pFile );
	else
		szRead = 0;

	// Calculate the Amount of Data read.
	if( pRead != NULL )
		*pRead = ( VFS_DWORD )szRead;

	return VFS_TRUE;
}

VFS_BOOL CStdIOFile::Write( const VFS_BYTE* pBuffer, VFS_DWORD dwToWrite, VFS_DWORD* pWritten )
{
	// Invalid File?
	if( m_pFile == NULL )
	{
		SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}

	// Invalid Buffer Pointer?
	if( dwToWrite > 0 && pBuffer == NULL )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Read-only?
	if( m_bReadOnly )
	{
		SetLastError( VFS_ERROR_PERMISSION_DENIED );
		return VFS_FALSE;
	}

	// Read.
	size_t szWritten;
	if( dwToWrite > 0 )
		szWritten = fwrite( pBuffer, 1, dwToWrite, m_pFile );
	else
		szWritten = 0;

	// Calculate the Amount of Data read.
	if( pWritten != NULL )
		*pWritten = ( VFS_DWORD )szWritten;

	return VFS_TRUE;
}

// Seek / Tell.
VFS_BOOL CStdIOFile::Seek( VFS_LONG lPosition, VFS_SeekOrigin eOrigin )
{
	// Invalid File?
	if( m_pFile == NULL )
	{
		SetLastError( VFS_ERROR_GENERIC );
		return VFS_FALSE;
	}

	return fseek( m_pFile, lPosition, eOrigin == VFS_SET ? SEEK_SET : ( eOrigin == VFS_CURRENT ? SEEK_CUR : SEEK_END ) ) == 0;
}

VFS_LONG CStdIOFile::Tell() const
{
	// Invalid File?
	if( m_pFile == NULL )
	{
		SetLastError( VFS_ERROR_GENERIC );
		return VFS_INVALID_LONG_VALUE;
	}

	return ftell( m_pFile );
}

// Sizing.
VFS_BOOL CStdIOFile::Resize( VFS_LONG lSize )
{
	return 0;//
}

VFS_LONG CStdIOFile::GetSize() const
{
	return VFS_GETSIZE( m_pFile );
}

// Open / Create a StdIOFile.
IFile* CStdIOFile::Open( const VFS_String& strAbsoluteFileName, VFS_DWORD dwFlags )
{
	// Try to open the File.
	CStdIOFile* pFile = new CStdIOFile( strAbsoluteFileName, VFS_TRUE );

	// File not found?
	if( !pFile->IsValid() )
	{
		delete pFile;
		SetLastError( VFS_ERROR_NOT_FOUND );
		return NULL;
	}

	// Read-only but write permission needed?
	if( pFile->m_bReadOnly && ( dwFlags & VFS_WRITE ) == VFS_WRITE )
	{
		delete pFile;
		SetLastError( VFS_ERROR_PERMISSION_DENIED );
		return NULL;
	}

	return pFile;
}

IFile* CStdIOFile::Create( const VFS_String& strAbsoluteFileName, VFS_DWORD dwFlags )
{
	// Try to open the File.
	CStdIOFile* pFile = new CStdIOFile( strAbsoluteFileName, VFS_FALSE );

	// File couldn't be created?
	if( !pFile->IsValid() )
	{
		delete pFile;
		return NULL;
	}

	// Read-only but write permission needed?
	// Usually, you'll have write permission whenever you create a File, so
	// probably we'd omit this one!?!
	if( pFile->m_bReadOnly && ( dwFlags & VFS_WRITE ) == VFS_WRITE )
	{
		delete pFile;
		SetLastError( VFS_ERROR_PERMISSION_DENIED );
		return NULL;
	}

	return pFile;
}

VFS_BOOL CStdIOFile::Exists( const VFS_String& strAbsoluteFileName )
{
	return VFS_EXISTS( strAbsoluteFileName );
}
