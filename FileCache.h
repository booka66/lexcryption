#ifndef FILECACHE_H
#define FILECACHE_H

#include <QDateTime>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

class FileCache : public QObject {
  Q_OBJECT

public:
  explicit FileCache(QObject *parent = nullptr);
  ~FileCache();

  QStringList findEncryptedFiles(const QString &startPath,
                                 bool useCache = true);
  void clearCache();
  void removeFromCache(const QString &path);
  void addToCache(const QString &path);
  void watchDirectory(const QString &path);

signals:
  void cacheUpdated();

private slots:
  void handleFileChanged(const QString &path);
  void handleDirectoryChanged(const QString &path);

private:
  struct CacheEntry {
    QString path;
    qint64 lastModified;
    qint64 size;

    bool isValid() const;
  };

  void loadCache();
  void saveCache();
  bool shouldSkipDirectory(const QString &path);
  void setupFileWatcher();

  QHash<QString, CacheEntry> cache;
  QString cacheFilePath;
  QSet<QString> excludedDirs;
  QFileSystemWatcher *fileWatcher;
  const int MAX_CACHE_AGE_DAYS = 7;
  QSet<QString> watchedDirectories;
};

#endif // FILECACHE_H
