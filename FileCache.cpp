#include "FileCache.h"
#include <QDir>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

FileCache::FileCache(QObject *parent) : QObject(parent) {
  // Set up cache file in app data location
  QString cacheDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(cacheDir);
  cacheFilePath = cacheDir + "/file_cache.json";

  // Initialize excluded directories
  excludedDirs = {
      "/proc",           "/sys",           "/dev",      "/run",
      "/snap",           "/var/run",       "/var/lock", "/private/var/vm",
      "/Library/Caches", "/System/Volumes"};

  setupFileWatcher();
  loadCache();
}

FileCache::~FileCache() {
  saveCache();
  delete fileWatcher;
}

void FileCache::setupFileWatcher() {
  fileWatcher = new QFileSystemWatcher(this);
  connect(fileWatcher, &QFileSystemWatcher::fileChanged, this,
          &FileCache::handleFileChanged);
  connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this,
          &FileCache::handleDirectoryChanged);
}

void FileCache::handleFileChanged(const QString &path) {
  if (path.endsWith(".senc")) {
    QFileInfo info(path);
    if (!info.exists()) {
      // File was deleted
      removeFromCache(path);
    } else {
      // File was modified
      addToCache(path);
    }
    emit cacheUpdated();
  }
}

void FileCache::handleDirectoryChanged(const QString &path) {
  // Scan directory for new .senc files
  QDir dir(path);
  QStringList sencFiles = dir.entryList(QStringList() << "*.senc", QDir::Files);

  for (const QString &file : sencFiles) {
    QString fullPath = dir.filePath(file);
    if (!cache.contains(fullPath)) {
      addToCache(fullPath);
    }
  }

  emit cacheUpdated();
}

void FileCache::watchDirectory(const QString &path) {
  if (!watchedDirectories.contains(path)) {
    fileWatcher->addPath(path);
    watchedDirectories.insert(path);

    // Watch subdirectories too
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
      QString subdir = it.next();
      if (!shouldSkipDirectory(subdir)) {
        fileWatcher->addPath(subdir);
        watchedDirectories.insert(subdir);
      }
    }
  }
}

void FileCache::removeFromCache(const QString &path) {
  cache.remove(path);
  saveCache();
}

void FileCache::addToCache(const QString &path) {
  QFileInfo info(path);
  if (info.exists() && path.endsWith(".senc")) {
    CacheEntry entry{path, info.lastModified().toSecsSinceEpoch(), info.size()};
    cache.insert(path, entry);
    fileWatcher->addPath(path);
    saveCache();
  }
}

bool FileCache::CacheEntry::isValid() const {
  QFileInfo info(path);
  return info.exists() &&
         info.lastModified().toSecsSinceEpoch() == lastModified &&
         info.size() == size;
}

void FileCache::loadCache() {
  QFile file(cacheFilePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  QJsonObject root = doc.object();

  // Check cache version and age
  qint64 cacheTimestamp = root["timestamp"].toDouble();
  if (QDateTime::currentSecsSinceEpoch() - cacheTimestamp >
      MAX_CACHE_AGE_DAYS * 86400) {
    // Cache too old, clear it
    cache.clear();
    return;
  }

  QJsonArray entries = root["entries"].toArray();
  for (const auto &entry : entries) {
    QJsonObject obj = entry.toObject();
    CacheEntry cacheEntry{obj["path"].toString(),
                          static_cast<qint64>(obj["lastModified"].toDouble()),
                          static_cast<qint64>(obj["size"].toDouble())};

    // Only add valid entries to cache
    if (cacheEntry.isValid()) {
      cache.insert(cacheEntry.path, cacheEntry);
    }
  }
}

void FileCache::saveCache() {
  QJsonObject root;
  root["timestamp"] = QDateTime::currentSecsSinceEpoch();

  QJsonArray entries;
  for (const auto &entry : cache) {
    QJsonObject obj;
    obj["path"] = entry.path;
    obj["lastModified"] = entry.lastModified;
    obj["size"] = entry.size;
    entries.append(obj);
  }

  root["entries"] = entries;

  QFile file(cacheFilePath);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(QJsonDocument(root).toJson());
  }
}

bool FileCache::shouldSkipDirectory(const QString &path) {
  // Skip system directories and hidden folders
  if (path.startsWith('.') || excludedDirs.contains(path)) {
    return true;
  }

  QFileInfo info(path);
  // Skip if no read permission
  if (!info.isReadable()) {
    return true;
  }

  return false;
}

QStringList FileCache::findEncryptedFiles(const QString &startPath,
                                          bool useCache) {
  QStringList results;

  // Watch the start directory and its subdirectories
  watchDirectory(startPath);

  if (useCache) {
    // First check cache for valid entries
    for (const auto &entry : cache) {
      if (entry.path.startsWith(startPath) && entry.isValid()) {
        results << entry.path;
      }
    }

    if (!results.isEmpty()) {
      return results;
    }
  }

  // Perform actual search if cache miss
  QDirIterator it(startPath, QStringList() << "*.senc",
                  QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);

  while (it.hasNext()) {
    QString filePath = it.next();
    QFileInfo info(filePath);

    if (shouldSkipDirectory(info.path())) {
      continue;
    }

    // Update cache
    CacheEntry entry{filePath, info.lastModified().toSecsSinceEpoch(),
                     info.size()};
    cache.insert(filePath, entry);
    fileWatcher->addPath(filePath); // Watch the file
    results << filePath;
  }

  // Save updated cache
  saveCache();
  return results;
}

void FileCache::clearCache() {
  cache.clear();
  saveCache();
}
