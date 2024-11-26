#ifndef SECUREVIEWER_H
#define SECUREVIEWER_H
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QMimeData>
#include <QPdfDocument>
#include <QPdfView>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoWidget>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class SecureViewer : public QMainWindow {
  Q_OBJECT
public:
  SecureViewer(QWidget *parent = nullptr);
  ~SecureViewer();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;

private slots:
  void openFile();
  void clearContent();
  void handleMediaError(QMediaPlayer::Error error, const QString &errorString);
  void handleUnencryptedFile();
  void saveAndEncrypt();
  void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
  QWidget *centralWidget;
  QVBoxLayout *mainLayout;
  QPushButton *decryptButton;
  QPushButton *clearButton;
  QPushButton *saveButton;
  QPushButton *uploadButton;
  QLineEdit *passwordInput;
  QStackedWidget *contentStack;
  QTextEdit *textViewer;
  QLabel *imageViewer;
  QMediaPlayer *videoPlayer;
  QVideoWidget *videoWidget;
  QTimer *autoDeleteTimer;
  std::filesystem::path tempDir;
  std::vector<std::filesystem::path> tempFiles;
  QString currentFilePath;
  QLabel *dropOverlay;
  QPixmap originalImage;
  QAudioOutput *audioOutput;
  QPdfDocument *pdfDocument;
  QPdfView *pdfViewer;
  QScrollArea *pdfScrollArea;
  QListWidget *fileList;
  QFileSystemWatcher *fsWatcher;
  QTimer *searchTimer;
  std::vector<fs::path> searchPaths;
  size_t currentSearchIndex;
  QStatusBar *mainStatusBar;
  QLabel *timerStatusLabel;
  QLabel *fileStatusLabel;
  QLabel *searchStatusLabel;

  std::filesystem::path createSecureTempDir();
  bool execCommand(const std::string &cmd, std::string &output);
  bool decryptFile(const std::filesystem::path &encryptedFile,
                   const QString &password);
  void cleanupTempFiles();
  std::string escapeShellArg(const std::string &arg);
  bool displayContent(const std::filesystem::path &filePath);
  void showDropOverlay(bool show);
  void setupDropOverlay();
  void saveAndEncryptFile(const QString &filePath);
  void resizeEvent(QResizeEvent *event) override;
  void updateImageScale();
  void requestPassword();
  void setupFileSidebar();
  void startFileSearch();
  void searchNextDirectory();
  void addEncFile(const fs::path &path);
  void updateTimerStatus();
  void updateFileStatus(const QString &status);
  void updateSearchStatus(const QString &status);
};

#endif
