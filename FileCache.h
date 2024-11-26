#ifndef FILECACHE_H
#define FILECACHE_H

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

class FileCache {
private:
  struct CacheEntry {
    QString path;
    qint64 lastModified;
    qint64 size;

    bool isValid() const;
  };

  QHash<QString, CacheEntry> cache;
  QString cacheFilePath;
  QSet<QString> excludedDirs;
  const int MAX_CACHE_AGE_DAYS = 7;

public:
  FileCache();
  void loadCache();
  void saveCache();
  bool shouldSkipDirectory(const QString &path);
  QStringList findEncryptedFiles(const QString &startPath,
                                 bool useCache = true);
};

#endif // FILECACHE_H
