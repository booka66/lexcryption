#ifndef SECUREVIEWER_H
#define SECUREVIEWER_H
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoWidget>
#include <filesystem>
#include <string>
#include <vector>

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
  void validatePassword(const QString &password);
  void handleMediaError(QMediaPlayer::Error error, const QString &errorString);
  void handleUnencryptedFile();
  void handleFileUpload(const QString &filePath);
  void saveAndEncrypt();

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
  bool isEditing;
  QString currentFilePath;
  bool contentModified;
  QLabel *dropOverlay;
  QPixmap originalImage;
  QAudioOutput *audioOutput;

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
};

#endif
