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
static VFS_String StripArchiveExtension( const VFS_String& strArchive );

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================
static VFS_String StripArchiveExtension( const VFS_String& strArchive )
{
	VFS_String ToRemove = VFS_String( VFS_TEXT( "." ) ) + ToLower( VFS_ARCHIVE_FILE_EXTENSION );
	VFS_String strResult = ToLower( strArchive );

	if( strResult.size() >= ToRemove.size() )
	{
		if( strResult.substr( strResult.size() - ToRemove.size(), ToRemove.size() ) == ToRemove )
		{
			strResult = strResult.substr( 0, strResult.size() - ToRemove.size() );
		}
	}

	return strResult;
}

//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
/// --- Archive File Class ---
// Constructor / Destructor.
CArchiveFile::CArchiveFile( const CArchive* pArchive, const VFS_String& strFileName, VFS_BOOL bOpen )
: IFile( pArchive ? ( StripArchiveExtension( pArchive->GetFileName() ) + VFS_PATH_SEPARATOR + strFileName ) : VFS_TEXT( "(invalid)" ) )
{
	// Invalid Archive Pointer?
	if( !pArchive )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		m_pArchive = NULL;
		return;
	}

	// Not really possible that this is called, but remember, we're paranoid.
	if( !bOpen )
	{
		SetLastError( VFS_ERROR_CANT_MANIPULATE_ARCHIVES );
		m_pArchive = NULL;
		return;
	}
	else
	{
		// The Archive must contain ourself.
		if( !pArchive->ContainsFile( strFileName ) )
		{
			SetLastError( VFS_ERROR_NOT_FOUND );
			m_pArchive = NULL;
			return;
		}

		// Set the Member Variables.
		m_pArchive = pArchive;
		m_pArchiveFile = pArchive->GetFile( strFileName );
		if( m_pArchiveFile == NULL )
		{
			m_pArchive = NULL;
			return;
		}

		// Activate the Archive.
		const_cast< CArchive* >( m_pArchive )->Activate();

		// Read in the File.
		if( !VFS_File_Seek( m_pArchive->GetFile(), m_pArchiveFile->dwDataOffset, VFS_SET ) )
		{
			m_pArchive = NULL;
			return;
		}
		g_FromBuffer.resize( m_pArchiveFile->dwCompressedSize );
		if( !VFS_File_Read( m_pArchive->GetFile(), &*g_FromBuffer.begin(), m_pArchiveFile->dwCompressedSize ) )
		{
			m_pArchive = NULL;
			return;
		}

		// Apply the Filters.
		VFS_EntityInfo Info;
		Info.bArchived = VFS_TRUE;
		Info.eType = VFS_FILE;
		Info.lSize = m_pArchiveFile->dwCompressedSize;
		Info.strPath = strFileName;
		VFS_Util_GetName( Info.strPath, Info.strName );

		for( VFS_DWORD dwFilter = 0; dwFilter != m_pArchive->GetHeader()->Filters.size(); dwFilter++ )
		{
			g_FromPos = 0;
			if( !m_pArchive->GetHeader()->Filters[ dwFilter ]->Decode( Reader, Writer, Info ) )
			{
				VFS_ErrorCode eError = VFS_GetLastError();
				if( eError == VFS_ERROR_NONE )
					eError = VFS_ERROR_GENERIC;
				SetLastError( eError );
				m_pArchive = NULL;
				return;
			}
			g_FromBuffer = g_ToBuffer;
			g_ToBuffer.clear();
		}

        // Et voila...
		m_Data = g_FromBuffer;

		// Set some Helper Variables...
		m_dwPos = 0;
	}
}

CArchiveFile::~CArchiveFile()
{
	// Just do nothing.
}

// Is the File valid?
VFS_BOOL CArchiveFile::IsValid() const
{
	return m_pArchive != NULL;
}

// Read / Write.
VFS_BOOL CArchiveFile::Read( VFS_BYTE* pBuffer, VFS_DWORD dwToRead, VFS_DWORD* pRead )
{
	// Invalid Parameter?
	if( pBuffer == NULL )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	// Reading after EOF is an Error.
	if( dwToRead > 0 && m_dwPos >= m_Data.size() )
		return VFS_FALSE;

	// Calculate the amount of Bytes to read.
	dwToRead = ( VFS_DWORD )min( m_Data.size() - m_dwPos, dwToRead );
	VFS_BYTE* pCurrent = ( VFS_BYTE* )( &*m_Data.begin() ) + m_dwPos;

    // Read.
	memcpy( pBuffer, pCurrent, dwToRead );

	// Update the File Pointers.
	m_dwPos += dwToRead;

	// Notify?
	if( pRead )
		*pRead = dwToRead;

	return VFS_TRUE;
}

VFS_BOOL CArchiveFile::Write( const VFS_BYTE* pBuffer, VFS_DWORD dwToWrite, VFS_DWORD* pWritten )
{
	SetLastError( VFS_ERROR_CANT_MANIPULATE_ARCHIVES );
	return VFS_FALSE;
}

// Seek / Tell.
VFS_BOOL CArchiveFile::Seek( VFS_LONG lPosition, VFS_SeekOrigin eOrigin )
{
	VFS_DWORD dwDesired = lPosition;

	// Modify the Position according to the Seek Origin.
	if( eOrigin == VFS_CURRENT )
		dwDesired += m_dwPos;
	else if( eOrigin == VFS_END )
		dwDesired += ( VFS_DWORD )m_Data.size();

    // Check the Position.
	if( dwDesired < 0 || dwDesired > m_Data.size() )
	{
		SetLastError( VFS_ERROR_INVALID_PARAMETER );
		return VFS_FALSE;
	}

	m_dwPos = dwDesired;

	return VFS_TRUE;
}

VFS_LONG CArchiveFile::Tell() const
{
	return m_dwPos;
}

// Sizing.
VFS_BOOL CArchiveFile::Resize( VFS_LONG lSize )
{
	SetLastError( VFS_ERROR_CANT_MANIPULATE_ARCHIVES );
	return VFS_FALSE;
}

VFS_LONG CArchiveFile::GetSize() const
{
	return ( VFS_LONG )m_Data.size();
}

// Open / Create an Archive File.
IFile* CArchiveFile::Open( const VFS_String& strAbsoluteFileName, VFS_DWORD dwFlags )
{
	// Doesn't the File exists?
	CArchive* pArchive;
	VFS_String strFileName;
	if( !Exists( strAbsoluteFileName, &pArchive, &strFileName ) )
	{
		SetLastError( VFS_ERROR_NOT_FOUND );
		return NULL;
	}

	// No write access!!!
	if( ( dwFlags & VFS_WRITE ) == VFS_WRITE )
	{
		SetLastError( VFS_ERROR_CANT_MANIPULATE_ARCHIVES );
		return NULL;
	}

	CArchiveFile* pFile = new CArchiveFile( pArchive, strFileName, VFS_TRUE );
	if( !pFile->IsValid() )
	{
		delete pFile;
		return NULL;
	}

	return pFile;
}

VFS_BOOL CArchiveFile::Exists( const VFS_String& strAbsoluteFileName, CArchive** ppArchive, VFS_String* pFileName )
{
	// Parse the File Name.
	// For instance if we have
	// /alpha/beta/gamma.txt
	// we'll check if there's an archive /alpha containing a file beta/gamma.txt,
	//		   and if there's an archive /alpha/beta containing a file gamma.txt.
	for( VFS_DWORD dwIndex = 0; dwIndex < strAbsoluteFileName.size(); dwIndex++ )
	{
		if( strAbsoluteFileName[ dwIndex ] == VFS_PATH_SEPARATOR && dwIndex != 0 &&
		    ( dwIndex != 2 || ( !isalpha( strAbsoluteFileName[ 0 ] ) || strAbsoluteFileName[ 1 ] != VFS_TEXT( ':' ) ) ) )
		{
			VFS_String strArchive = ToLower( strAbsoluteFileName.substr( 0, dwIndex ) );
			VFS_String strSecond = ToLower( strAbsoluteFileName.substr( dwIndex + 1 ) );

			if( VFS_File_Exists( strArchive + VFS_TEXT( "." ) + VFS_ARCHIVE_FILE_EXTENSION ) )
			{
				// Open the Archive if it isn't open yet.
				if( GetOpenArchives().find( strArchive ) == GetOpenArchives().end() )
				{
					CArchive* pArchive = CArchive::Open( strArchive );
					if( pArchive == NULL )
						return VFS_FALSE;
					GetOpenArchives()[ strArchive ] = pArchive;
				}

				// Check if it contains such a File.
				if( GetOpenArchives()[ strArchive ]->ContainsFile( strSecond ) )
				{
					// Success.
					if( ppArchive )
						*ppArchive = GetOpenArchives()[ strArchive ];
					if( pFileName )
						*pFileName = strSecond;

					return VFS_TRUE;
				}
			}
		}
	}

	return VFS_FALSE;
}