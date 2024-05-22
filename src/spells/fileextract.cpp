#include "spellbook.h"

#include <QDialog>
#include <QFileDialog>
#include <QSettings>

#include "gamemanager.h"
#include "libfo76utils/src/common.hpp"
#include "libfo76utils/src/filebuf.hpp"
#include "libfo76utils/src/material.hpp"

#ifdef Q_OS_WIN32
#  include <direct.h>
#else
#  include <sys/stat.h>
#endif

// Brief description is deliberately not autolinked to class Spell
/*! \file fileextract.cpp
 * \brief Resource file extraction spell (spResourceFileExtract)
 *
 * All classes here inherit from the Spell class.
 */

//! Extract a resource file
class spResourceFileExtract final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Extract File" ); }
	QString page() const override final { return Spell::tr( "" ); }
	QIcon icon() const override final
	{
		return QIcon();
	}
	bool constant() const override final { return true; }
	bool instant() const override final { return true; }

	static bool is_Applicable( const NifModel * nif, const NifItem * item )
	{
		NifValue::Type	vt = item->valueType();
		if ( vt != NifValue::tStringIndex && vt != NifValue::tSizedString ) {
			if ( !( nif->checkVersion( 0x14010003, 0 ) && ( vt == NifValue::tString || vt == NifValue::tFilePath ) ) )
				return false;
		}
		do {
			if ( item->parent() && nif && nif->getBSVersion() >= 130 ) {
				if ( item->name() == "Name" && ( item->parent()->name() == "BSLightingShaderProperty" || item->parent()->name() == "BSEffectShaderProperty" ) )
					break;		// Fallout 4, 76 or Starfield material
			}
			if ( item->parent() && item->parent()->name() == "Textures" )
				break;
			if ( item->name() == "Path" || item->name() == "Mesh Path" || item->name().startsWith( "Texture " ) )
				break;
			return false;
		} while ( false );
		return !( nif->resolveString( item ).isEmpty() );
	}

	static std::string getNifItemFilePath( NifModel * nif, const NifItem * item );
	static std::string getOutputDirectory();
	static void writeFileWithPath( const char * fileName, const char * buf, qsizetype bufSize );

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		const NifItem * item = nif->getItem( index );
		return ( item && is_Applicable( nif, item ) );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

std::string spResourceFileExtract::getNifItemFilePath( NifModel * nif, const NifItem * item )
{
	const char *	archiveFolder = nullptr;
	const char *	extension = nullptr;

	quint32	bsVersion = nif->getBSVersion();
	if ( item->parent() && bsVersion >= 130 && item->name() == "Name" ) {
		if ( item->parent()->name() == "BSLightingShaderProperty" ) {
			archiveFolder = "materials/";
			extension = ( bsVersion < 160 ? ".bgsm" : ".mat" );
		} else if ( item->parent()->name() == "BSEffectShaderProperty" ) {
			archiveFolder = "materials/";
			extension = ( bsVersion < 160 ? ".bgem" : ".mat" );
		}
	} else if ( ( item->parent() && item->parent()->name() == "Textures" ) || item->name().contains( "Texture" ) || ( bsVersion >= 160 && item->name() == "Path" ) ) {
		archiveFolder = "textures/";
		extension = ".dds";
	} else if ( bsVersion >= 160 && item->name() == "Mesh Path" ) {
		archiveFolder = "geometries/";
		extension = ".mesh";
	}

	QString	filePath( nif->resolveString( item ) );
	if ( filePath.isEmpty() )
		return std::string();
	return Game::GameManager::get_full_path( filePath, archiveFolder, extension );
}

std::string spResourceFileExtract::getOutputDirectory()
{
	QSettings	settings;
	QString	key = QString( "Spells//Extract File/Last File Path" );
	QString	dstPath( settings.value( key ).toString() );
	{
		QFileDialog	dialog;
		dialog.setFileMode( QFileDialog::Directory );
		if ( !dstPath.isEmpty() )
			dialog.selectFile( dstPath );
		if ( !dialog.exec() )
			return std::string();
		dstPath = dialog.selectedFiles().at( 0 );
	}
	if ( dstPath.isEmpty() )
		return std::string();
	settings.setValue( key, QVariant(dstPath) );

	std::string	fullPath( dstPath.replace( QChar('\\'), QChar('/') ).toStdString() );
	if ( !fullPath.ends_with( '/' ) )
		fullPath += '/';
	return fullPath;
}

void spResourceFileExtract::writeFileWithPath( const char * fileName, const char * buf, qsizetype bufSize )
{
	if ( bufSize < 0 )
		return;
	OutputFile *	f = nullptr;
	try {
		f = new OutputFile( fileName, 0 );
	} catch ( ... ) {
		std::string	pathName;
		size_t	pathOffs = 0;
		while (true) {
			pathName = fileName;
			pathOffs = pathName.find( '/', pathOffs );
			if ( pathOffs == std::string::npos )
				break;
			pathName.resize( pathOffs );
			pathOffs++;
#ifdef Q_OS_WIN32
			(void) _mkdir( pathName.c_str() );
#else
			(void) mkdir( pathName.c_str(), 0755 );
#endif
		}
		f = new OutputFile( fileName, 0 );
	}

	try {
		f->writeData( buf, size_t(bufSize) );
	}
	catch ( ... ) {
		delete f;
		throw;
	}
	delete f;
}

QModelIndex spResourceFileExtract::cast( NifModel * nif, const QModelIndex & index )
{
	if ( !nif )
		return index;
	Game::GameMode	game = Game::GameManager::get_game( nif->getVersionNumber(), nif->getUserVersion(), nif->getBSVersion() );

	const NifItem * item = nif->getItem( index );
	if ( !item )
		return index;

	std::string	filePath( getNifItemFilePath( nif, item ) );
	if ( filePath.empty() )
		return index;

	std::string	matFileData;
	try {
		if ( nif->getBSVersion() >= 160 && filePath.ends_with( ".mat" ) && filePath.starts_with( "materials/" ) ) {
			CE2MaterialDB *	materials = Game::GameManager::materials( game );
			if ( materials ) {
				(void) materials->loadMaterial( filePath );
				materials->getJSONMaterial( matFileData, filePath );
			}
			if ( matFileData.empty() )
				return index;
		} else if ( Game::GameManager::find_file( game, QString::fromStdString( filePath ), nullptr, nullptr ).isEmpty() ) {
			return index;
		}

		std::string	fullPath( getOutputDirectory() );
		if ( fullPath.empty() )
			return index;
		fullPath += filePath;

		if ( !matFileData.empty() ) {
			matFileData += '\n';
			writeFileWithPath( fullPath.c_str(), matFileData.c_str(), qsizetype(matFileData.length()) );
		} else {
			QByteArray	fileData;
			if ( Game::GameManager::get_file( fileData, game, filePath ) )
				writeFileWithPath( fullPath.c_str(), fileData.data(), fileData.size() );
		}
	} catch ( std::exception & e ) {
		QMessageBox::critical( nullptr, "NifSkope error", QString("Error extracting file: %1" ).arg( e.what() ) );
	}
	return index;
}

REGISTER_SPELL( spResourceFileExtract )

//! Extract all resource files
class spExtractAllResources final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Extract Resource Files" ); }
	QString page() const override final { return Spell::tr( "" ); }
	QIcon icon() const override final
	{
		return QIcon();
	}
	bool constant() const override final { return true; }
	bool instant() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif && !index.isValid() );
	}

	static void findPaths( std::set< std::string > & fileSet, NifModel * nif, const NifItem * item );
	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

void spExtractAllResources::findPaths( std::set< std::string > & fileSet, NifModel * nif, const NifItem * item )
{
	if ( spResourceFileExtract::is_Applicable( nif, item ) ) {
		std::string	filePath( spResourceFileExtract::getNifItemFilePath( nif, item ) );
		if ( !filePath.empty() )
			fileSet.insert( filePath );
	}

	for ( int i = 0; i < item->childCount(); i++ ) {
		if ( item->child( i ) )
			findPaths( fileSet, nif, item->child( i ) );
	}
}

QModelIndex spExtractAllResources::cast( NifModel * nif, const QModelIndex & index )
{
	if ( !nif )
		return index;
	Game::GameMode	game = Game::GameManager::get_game( nif->getVersionNumber(), nif->getUserVersion(), nif->getBSVersion() );

	std::set< std::string >	fileSet;
	for ( int b = 0; b < nif->getBlockCount(); b++ ) {
		const NifItem * item = nif->getBlockItem( quint32(b) );
		if ( item )
			findPaths( fileSet, nif, item );
	}
	if ( fileSet.begin() == fileSet.end() )
		return index;

	std::string	dstPath( spResourceFileExtract::getOutputDirectory() );
	if ( dstPath.empty() )
		return index;

	std::string	matFileData;
	std::string	fullPath;
	QByteArray	fileData;
	try {
		for ( std::set< std::string >::const_iterator i = fileSet.begin(); i != fileSet.end(); i++ ) {
			matFileData.clear();
			if ( nif->getBSVersion() >= 160 && i->ends_with( ".mat" ) && i->starts_with( "materials/" ) ) {
				CE2MaterialDB *	materials = Game::GameManager::materials( game );
				if ( materials ) {
					(void) materials->loadMaterial( *i );
					materials->getJSONMaterial( matFileData, *i );
				}
				if ( matFileData.empty() )
					continue;
			} else if ( Game::GameManager::find_file( game, QString::fromStdString( *i ), nullptr, nullptr ).isEmpty() ) {
				continue;
			}

			fullPath = dstPath;
			fullPath += *i;
			if ( !matFileData.empty() ) {
				matFileData += '\n';
				spResourceFileExtract::writeFileWithPath( fullPath.c_str(), matFileData.c_str(), qsizetype(matFileData.length()) );
			} else if ( Game::GameManager::get_file( fileData, game, *i ) ) {
				spResourceFileExtract::writeFileWithPath( fullPath.c_str(), fileData.data(), fileData.size() );
			}
		}
	} catch ( std::exception & e ) {
		QMessageBox::critical( nullptr, "NifSkope error", QString("Error extracting file: %1" ).arg( e.what() ) );
	}
	return index;
}

REGISTER_SPELL( spExtractAllResources )

