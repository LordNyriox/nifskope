#include "gamemanager.h"

#include "bsa.h"
#include "message.h"
#include "bsrefl.hpp"

#include <QSettings>
#include <QCoreApplication>
#include <QProgressDialog>
#include <QDir>
#include <QMessageBox>

#include <dirent.h>

namespace Game
{

struct BA2Files {
	std::map< GameMode, BA2File* >	archives;
	~BA2Files();
	const BA2File* operator[](GameMode game) const
	{
		std::map< GameMode, BA2File* >::const_iterator	i = archives.find(game);
		if (i == archives.end())
			return nullptr;
		return i->second;
	}
	void open_folders(GameMode game, const QStringList& folders);
	void close_archive(GameMode game);
	void close_all();
	bool add_folder(GameMode game, const char* pathName, bool ignoreErrors);
};

BA2Files::~BA2Files()
{
	close_all();
}

void BA2Files::open_folders(GameMode game, const QStringList& folders)
{
	close_archive(game);
	archives[game] = nullptr;
	std::vector< std::string >	tmp;
	for (const auto& s : folders) {
		if (!s.isEmpty())
			tmp.push_back(s.toStdString());
	}
	if (tmp.size() < 1)
		return;
	archives[game] = new BA2File();
	std::vector< std::string >	excludePatterns;
	excludePatterns.emplace_back(".nif");
	size_t	archivesLoaded = 0;
	for (size_t i = tmp.size(); i-- > 0; ) {
		try {
			archives[game]->loadArchivePath(tmp[i].c_str(),
											nullptr, &excludePatterns);
			archivesLoaded++;
		} catch (FO76UtilsError& e) {
			QMessageBox::critical(nullptr, "NifSkope error", QString("Error opening archive path '%1': %2").arg(tmp[i].c_str()).arg(e.what()));
		}
	}
	if (!archivesLoaded) {
		close_archive(game);
		archives[game] = nullptr;
	}
}

void BA2Files::close_archive(GameMode game)
{
	std::map< GameMode, BA2File* >::iterator	i = archives.find(game);
	if (i != archives.end()) {
		if (i->second)
			delete i->second;
		i->second = nullptr;
	}
}

void BA2Files::close_all()
{
	for (std::map< GameMode, BA2File* >::iterator i = archives.begin(); i != archives.end(); i++)
	{
		if (i->second)
			delete i->second;
		i->second = nullptr;
	}
}

bool BA2Files::add_folder(GameMode game, const char* pathName, bool ignoreErrors)
{
	if (!archives[game])
		archives[game] = new BA2File();
	std::vector< std::string >	excludePatterns;
	excludePatterns.emplace_back(".nif");
	try {
		archives[game]->loadArchivePath(pathName, nullptr, &excludePatterns);
	} catch (FO76UtilsError& e) {
		if (!ignoreErrors)
			QMessageBox::critical(nullptr, "NifSkope error", QString("Error opening archive path '%1': %2").arg(pathName).arg(e.what()));
		return false;
	}
	return true;
}

static BA2Files	ba2Files;

static const auto GAME_PATHS = QString("Game Paths");
static const auto GAME_FOLDERS = QString("Game Folders");
static const auto GAME_STATUS = QString("Game Status");
static const auto GAME_MGR_VER = QString("Game Manager Version");

static CE2MaterialDB	starfield_materials;
static bool	have_materials_cdb = false;

QString registry_game_path( const QString& key )
{
#ifdef Q_OS_WIN32
	QString data_path;
	QSettings cfg(key, QSettings::Registry32Format);
	data_path = cfg.value("Installed Path").toString(); // Steam
	if ( data_path.isEmpty() )
		data_path = cfg.value("Path").toString(); // Microsoft Uninstall
	// Remove encasing quotes
	data_path.remove('"');
	if ( data_path.isEmpty() )
		return {};

	QDir data_path_dir(data_path);
	if ( data_path_dir.exists() )
		return QDir::cleanPath(data_path);

#else
	(void) key;
#endif
	return {};
}

QStringList existing_folders(GameMode game, QString path)
{
	// TODO: Restore the minimal previous support for detecting Civ IV, etc.
	if ( game == OTHER )
		return {};

	QStringList folders;
	for ( const auto& f : FOLDERS.value(game, {}) ) {
		QDir dir(QString("%1/%2").arg(path).arg(DATA.value(game, "")));
		if ( dir.exists(f) )
			folders.append(QFileInfo(dir, f).absoluteFilePath());
	}

	return folders;
}

GameManager::GameInfo get_game_info(GameMode game)
{
	GameManager::GameInfo info;
	info.id = game;
	info.name = StringForMode(game);
	info.path = registry_game_path(KEY.value(game, {}));
	return info;
}

QString StringForMode( GameMode game )
{
	if ( game >= NUM_GAMES )
		return {};

	return STRING.value(game, "");
}

GameMode ModeForString( QString game )
{
	return STRING.key(game, OTHER);
}

QProgressDialog* prog_dialog( QString title )
{
	QProgressDialog* dlg = new QProgressDialog(title, {}, 0, NUM_GAMES);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();
	return dlg;
}

void process( QProgressDialog* dlg, int i )
{
	if ( dlg ) {
		dlg->setValue(i);
		QCoreApplication::processEvents();
	}
}

GameManager::GameManager()
{
	QSettings settings;
	int manager_version = settings.value(GAME_MGR_VER, 0).toInt();
	if ( manager_version == 0 ) {
		auto dlg = prog_dialog("Initializing the Game Manager");
		// Initial game manager settings
		init_settings(manager_version, dlg);
		dlg->close();
	}

	if ( manager_version == 1 ) {
		update_settings(manager_version);
	}

	load();
}

GameMode GameManager::get_game( uint32_t version, uint32_t user, uint32_t bsver )
{
	switch ( bsver ) {
	case 0:
		break;
	case BSSTREAM_1:
	case BSSTREAM_3:
	case BSSTREAM_4:
	case BSSTREAM_5:
	case BSSTREAM_6:
	case BSSTREAM_7:
	case BSSTREAM_8:
	case BSSTREAM_9:
		return OBLIVION;
	case BSSTREAM_11:
		if ( user == 10 || version == 0x14000005 ) // TODO: Enumeration
			return OBLIVION;
		else if ( user == 11 )
			return FALLOUT_3NV;
		return OTHER;
	case BSSTREAM_14:
	case BSSTREAM_16:
	case BSSTREAM_21:
	case BSSTREAM_24:
	case BSSTREAM_25:
	case BSSTREAM_26:
	case BSSTREAM_27:
	case BSSTREAM_28:
	case BSSTREAM_30:
	case BSSTREAM_31:
	case BSSTREAM_32:
	case BSSTREAM_33:
	case BSSTREAM_34:
		return FALLOUT_3NV;
	case BSSTREAM_83:
		return SKYRIM;
	case BSSTREAM_100:
		return SKYRIM_SE;
	case BSSTREAM_130:
		return FALLOUT_4;
	case BSSTREAM_155:
		return FALLOUT_76;
	case BSSTREAM_172:
		return STARFIELD;
	default:
		break;
	};

	// NOTE: Morrowind shares a version with other games (Freedom Force, etc.)
	if ( version == 0x04000002 )
		return MORROWIND;

	return OTHER;
}

GameManager* GameManager::get()
{
	static auto instance{new GameManager{}};
	return instance;
}

void GameManager::init_settings( int& manager_version, QProgressDialog* dlg ) const
{
	QSettings settings;
	QVariantMap paths, folders, status;
	for ( int g = 0; g < NUM_GAMES; g++ ) {
		process(dlg, g);
		auto game = get_game_info(GameMode(g));
		if ( game.path.isEmpty() )
			continue;
		paths.insert(game.name, game.path);
		folders.insert(game.name, existing_folders(game.id, game.path));

		// Game Enabled Status
		status.insert(game.name, !game.path.isEmpty());
	}

	settings.setValue(GAME_PATHS, paths);
	settings.setValue(GAME_FOLDERS, folders);
	settings.setValue(GAME_STATUS, status);
	settings.setValue(GAME_MGR_VER, ++manager_version);
}

void GameManager::update_settings( int& manager_version, QProgressDialog * dlg ) const
{
	QSettings settings;
	QVariantMap folders;

	for ( int g = 0; g < NUM_GAMES; g++ ) {
		process(dlg, g);
		auto game = get_game_info(GameMode(g));
		if ( game.path.isEmpty() )
			continue;

		if ( manager_version == 1 )
			folders.insert(game.name, existing_folders(game.id, game.path));
	}

	if ( manager_version == 1 ) {
		settings.setValue(GAME_FOLDERS, folders);
		manager_version++;
	}

	settings.setValue(GAME_MGR_VER, manager_version);
}

QString GameManager::path( const GameMode game )
{
	return get()->game_paths.value(game, {});
}

QString GameManager::data( const GameMode game )
{
	return path(game) + "/" + DATA[game];
}

QStringList GameManager::folders( const GameMode game )
{
	if ( game == FALLOUT_3NV )
		return folders(FALLOUT_NV) + folders(FALLOUT_3);
	if ( status(game) )
		return get()->game_folders.value(game, {});
	return {};
}

bool GameManager::status(const GameMode game)
{
	if ( game == FALLOUT_3NV )
		return status(FALLOUT_3) || status(FALLOUT_NV);
	return get()->game_status.value(game, false);
}

std::string GameManager::get_full_path(const QString& name, const char* archive_folder, const char* extension)
{
	if (name.isEmpty())
		return std::string();
	std::string	s = name.toLower().replace('\\', '/').toStdString();
	if (archive_folder && *archive_folder) {
		std::string	d(archive_folder);
		if (!d.ends_with('/'))
			d += '/';
		size_t	n = 0;
		for ( ; n < s.length(); n = n + d.length()) {
			n = s.find(d, n);
			if (n == 0 || n == std::string::npos || s[n - 1] == '/')
				break;
		}
		if (n == std::string::npos || n >= s.length())
			s.insert(0, d);
		else if (n)
			s.erase(0, n);
	}
	if (extension && *extension && !s.ends_with(extension)) {
		size_t	n = s.rfind('.');
		if (n != std::string::npos) {
			size_t	d = s.rfind('/');
			if (d == std::string::npos || d < n)
				s.resize(n);
		}
		s += extension;
	}
	return s;
}

QString GameManager::find_file(const GameMode game, const QString& path, const char* archiveFolder, const char* extension)
{
	std::string	fullPath(get_full_path(path, archiveFolder, extension));
	const BA2File*	ba2File = ba2Files[game];
	if (!ba2File) {
		try
		{
			ba2Files.open_folders(game, folders(game));
			ba2File = ba2Files[game];
		}
		catch (...)
		{
		}
		if (!ba2File) {
			qWarning() << "Archive(s) not loaded for game " << STRING[game];
			return QString();
		}
	}
	if (ba2File->findFile(fullPath))
		return QString::fromStdString(fullPath);
	return QString();
}

bool GameManager::get_file(std::vector< unsigned char >& data, const GameMode game, const std::string& fullPath)
{
	const BA2File*	ba2File = ba2Files[game];
	if (!ba2File) {
		try
		{
			ba2Files.open_folders(game, folders(game));
			ba2File = ba2Files[game];
		}
		catch (...)
		{
		}
		if (!ba2File) {
			qWarning() << "Archive(s) not loaded for game " << STRING[game];
			return false;
		}
	}
	if (!ba2File->findFile(fullPath)) {
		qWarning() << "File '" << fullPath.c_str() << "' not found in archives";
		return false;
	}
	ba2File->extractFile(data, fullPath);
	return true;
}

bool GameManager::get_file(std::vector< unsigned char >& data, const GameMode game, const QString& path, const char* archiveFolder, const char* extension)
{
	std::string	fullPath(get_full_path(path, archiveFolder, extension));
	return get_file(data, game, fullPath);
}

bool GameManager::get_file(QByteArray& data, const GameMode game, const QString& path, const char* archiveFolder, const char* extension)
{
	std::string	fullPath(get_full_path(path, archiveFolder, extension));
	std::vector< unsigned char >	tmpData;
	if (!get_file(tmpData, game, fullPath))
		return false;
	data.resize(tmpData.size());
	std::memcpy(data.data(), tmpData.data(), tmpData.size());
	return true;
}

const CE2MaterialDB* GameManager::materials(const GameMode game)
{
	if ( game == STARFIELD ) {
		if ( !have_materials_cdb ) {
			std::vector< unsigned char >	cdb_data;
			if (get_file(cdb_data, game, std::string(BSReflStream::getDefaultMaterialDBPath())))
				starfield_materials.loadCDBFile(cdb_data.data(), cdb_data.size());
			have_materials_cdb = true;
		}
		if ( have_materials_cdb )
			return &starfield_materials;
	}
	return nullptr;
}

void GameManager::close_archives()
{
	ba2Files.close_all();
}

void GameManager::close_materials()
{
	if ( have_materials_cdb ) {
		have_materials_cdb = false;
		starfield_materials.~CE2MaterialDB();
		(void) new( &starfield_materials ) CE2MaterialDB();
	}
}

bool GameManager::add_temp_path(const GameMode game, const char* pathName, bool ignoreErrors)
{
	if ( !pathName )
		return false;
	std::string	tmpPath( pathName );
	while ( true ) {
		if ( tmpPath.empty() )
			return false;
		std::uint32_t	extStr = 0U;
		if ( tmpPath.length() > 4 ) {
			extStr = FileBuffer::readUInt32Fast( tmpPath.c_str() + (tmpPath.length() - 4) );
			extStr = extStr | ((extStr >> 1) & 0x20202020U);
		}
		if ( FileBuffer::checkType( extStr, ".nif" ) ) {
			size_t	n1 = tmpPath.rfind('/');
			size_t	n2 = tmpPath.rfind('\\');
			if ( n1 == std::string::npos )
				n1 = 0;
			if ( n2 == std::string::npos )
				n2 = 0;
			tmpPath.resize( std::max( n1, n2 ) );
			continue;
		}
		if ( FileBuffer::checkType( extStr, ".ba2" ) || FileBuffer::checkType( extStr, ".bsa" ) )
			break;
		DIR*	d = opendir( tmpPath.c_str() );
		if ( d ) {
			closedir( d );
			break;
		}
	}
	if ( !ba2Files[game] )
		ba2Files.open_folders( game, folders(game) );
	return ba2Files.add_folder( game, tmpPath.c_str(), ignoreErrors );
}

QStringList GameManager::find_folders(const GameMode game)
{
	return existing_folders(game, get()->game_paths.value(game, {}));
}

void GameManager::save() const
{
	QSettings settings;
	QVariantMap paths, folders, status;
	for ( const auto& p : game_paths.toStdMap() )
		paths.insert(StringForMode(p.first), p.second);

	for ( const auto& f : game_folders.toStdMap() )
		folders.insert(StringForMode(f.first), f.second);

	for ( const auto& s : game_status.toStdMap() )
		status.insert(StringForMode(s.first), s.second);

	settings.setValue(GAME_PATHS, paths);
	settings.setValue(GAME_FOLDERS, folders);
	settings.setValue(GAME_STATUS, status);
}

void GameManager::load()
{
	QMutexLocker locker(&mutex);
	QSettings settings;
	for ( const auto& p : settings.value(GAME_PATHS).toMap().toStdMap() )
		game_paths[ModeForString(p.first)] = p.second.toString();

	for ( const auto& f : settings.value(GAME_FOLDERS).toMap().toStdMap() )
		game_folders[ModeForString(f.first)] = f.second.toStringList();

	for ( const auto& s : settings.value(GAME_STATUS).toMap().toStdMap() )
		game_status[ModeForString(s.first)] = s.second.toBool();
}

void GameManager::clear()
{
	QMutexLocker locker(&mutex);
	game_paths.clear();
	game_folders.clear();
	game_status.clear();
}

void GameManager::insert_game( const GameMode game, const QString& path )
{
	QMutexLocker locker(&mutex);
	game_paths.insert(game, path);
}

void GameManager::insert_folders( const GameMode game, const QStringList& list )
{
	QMutexLocker locker(&mutex);
	game_folders.insert(game, list);
}

void GameManager::insert_status( const GameMode game, bool status )
{
	QMutexLocker locker(&mutex);
	game_status.insert(game, status);
}

} // end namespace Game
