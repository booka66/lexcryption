#include "FileCache.h"
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

bool FileCache::CacheEntry::isValid() const {
  QFileInfo info(path);
  return info.exists() &&
         info.lastModified().toSecsSinceEpoch() == lastModified &&
         info.size() == size;
}

FileCache::FileCache() {
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

  loadCache();
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
    results << filePath;
  }

  // Save updated cache
  saveCache();
  return results;
}
