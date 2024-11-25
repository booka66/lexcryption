#include "SecureViewer.h"
#include <QApplication>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QScreen>
#include <QStackedWidget>
#include <QStyle>
#include <QVideoWidget>
#include <array>
#include <cstdio>
#include <fstream>
#include <random>

namespace fs = std::filesystem;

SecureViewer::SecureViewer(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Secure File Viewer");
  setMinimumSize(800, 600);
  setAcceptDrops(true); // Enable drag and drop

  centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);
  mainLayout = new QVBoxLayout(centralWidget);

  auto *buttonLayout = new QHBoxLayout();
  decryptButton = new QPushButton("Decrypt File", this);
  decryptButton->setEnabled(false);
  clearButton = new QPushButton("Clear", this);
  saveButton = new QPushButton("Save & Encrypt", this);
  uploadButton = new QPushButton("Encrypt File", this);
  passwordInput = new QLineEdit(this);
  passwordInput->setPlaceholderText(
      "Enter decryption password (min 6 characters)");
  passwordInput->setEchoMode(QLineEdit::Password);

  buttonLayout->addWidget(decryptButton);
  buttonLayout->addWidget(uploadButton);
  buttonLayout->addWidget(passwordInput);
  buttonLayout->addWidget(clearButton);
  buttonLayout->addWidget(saveButton);

  contentStack = new QStackedWidget(this);
  textViewer = new QTextEdit(this);
  textViewer->setReadOnly(false);
  imageViewer = new QLabel(this);
  imageViewer->setAlignment(Qt::AlignCenter);
  imageViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  videoPlayer = new QMediaPlayer(this);
  videoWidget = new QVideoWidget(this);
  videoPlayer->setVideoOutput(videoWidget);

  contentStack->addWidget(textViewer);
  contentStack->addWidget(imageViewer);
  contentStack->addWidget(videoWidget);

  mainLayout->addLayout(buttonLayout);
  mainLayout->addWidget(contentStack);

  autoDeleteTimer = new QTimer(this);
  int min = 60 * 1000;
  int timeout = 10 * min;
  autoDeleteTimer->setInterval(timeout);
  autoDeleteTimer->setSingleShot(true);

  setupDropOverlay();

  connect(decryptButton, &QPushButton::clicked, this, &SecureViewer::openFile);
  connect(clearButton, &QPushButton::clicked, this,
          &SecureViewer::clearContent);
  connect(uploadButton, &QPushButton::clicked, this,
          &SecureViewer::handleUnencryptedFile);
  connect(autoDeleteTimer, &QTimer::timeout, this, &SecureViewer::clearContent);
  connect(passwordInput, &QLineEdit::textChanged, this,
          &SecureViewer::validatePassword);
  connect(videoPlayer, &QMediaPlayer::errorOccurred, this,
          &SecureViewer::handleMediaError);
  connect(saveButton, &QPushButton::clicked, this,
          &SecureViewer::saveAndEncrypt);
  connect(textViewer, &QTextEdit::textChanged,
          [this]() { contentModified = true; });

  tempDir = createSecureTempDir();
  isEditing = true;
  contentModified = false;
}

void SecureViewer::setupDropOverlay() {
  dropOverlay = new QLabel(this);
  dropOverlay->setAlignment(Qt::AlignCenter);
  dropOverlay->setText("Drop file here");
  dropOverlay->setStyleSheet("QLabel {"
                             "  background-color: rgba(0, 120, 215, 0.7);"
                             "  color: white;"
                             "  border: 4px dashed #ffffff;"
                             "  border-radius: 10px;"
                             "  font-size: 24px;"
                             "  padding: 20px;"
                             "}");
  dropOverlay->hide();
  dropOverlay->setGeometry(rect());
}

void SecureViewer::showDropOverlay(bool show) {
  if (show) {
    dropOverlay->setGeometry(rect());
    dropOverlay->raise();
    dropOverlay->show();
  } else {
    dropOverlay->hide();
  }
}

void SecureViewer::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
    showDropOverlay(true);
  }
}

void SecureViewer::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void SecureViewer::dragLeaveEvent(QDragLeaveEvent *event) {
  showDropOverlay(false);
  event->accept();
}

void SecureViewer::dropEvent(QDropEvent *event) {
  showDropOverlay(false);
  const QMimeData *mimeData = event->mimeData();

  if (mimeData->hasUrls()) {
    QList<QUrl> urlList = mimeData->urls();
    if (!urlList.isEmpty()) {
      QString filePath = urlList.first().toLocalFile();
      if (filePath.endsWith(".senc", Qt::CaseInsensitive)) {
        // Handle encrypted file
        if (!passwordInput->text().isEmpty()) {
          decryptFile(filePath.toStdString(), passwordInput->text());
        } else {
          QMessageBox::warning(this, "Warning",
                               "Please enter the decryption password first!");
        }
      } else {
        // Handle unencrypted file - just display it
        currentFilePath = filePath;
        if (displayContent(filePath.toStdString())) {
          isEditing = true;
          contentModified = true;
        }
      }
    }
  }

  event->acceptProposedAction();
}

void SecureViewer::handleUnencryptedFile() {
  QString filePath =
      QFileDialog::getOpenFileName(this, "Select File", "", "All Files (*)");

  if (!filePath.isEmpty()) {
    currentFilePath = filePath;
    if (displayContent(filePath.toStdString())) {
      isEditing = true;
      contentModified = true;
    }
  }
}

void SecureViewer::handleFileUpload(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(this, "Error", "Could not open the selected file!");
    return;
  }

  // For text files, load into text editor
  if (filePath.endsWith(".txt", Qt::CaseInsensitive) ||
      filePath.endsWith(".md", Qt::CaseInsensitive) ||
      filePath.endsWith(".csv", Qt::CaseInsensitive)) {
    QTextStream in(&file);
    textViewer->setText(in.readAll());
    contentStack->setCurrentWidget(textViewer);
    isEditing = true;
    contentModified = true;
  } else {
    // For other files, copy to temporary location and prepare for encryption
    QString tempFile =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) +
        "/" + QFileInfo(filePath).fileName();

    if (QFile::copy(filePath, tempFile)) {
      currentFilePath = tempFile;
      QMessageBox::information(this, "File Ready",
                               "File has been prepared for encryption. Click "
                               "'Save & Encrypt' to encrypt the file.");
    } else {
      QMessageBox::critical(this, "Error",
                            "Failed to prepare file for encryption!");
    }
  }

  file.close();
}

void SecureViewer::validatePassword(const QString &password) {
  decryptButton->setEnabled(password.length() >= 6);
}

void SecureViewer::handleMediaError([[maybe_unused]] QMediaPlayer::Error error,
                                    const QString &errorString) {
  QMessageBox::warning(this, "Media Error",
                       QString("Error playing media: %1").arg(errorString));
}

bool SecureViewer::displayContent(const fs::path &filePath) {
  QString extension =
      QString::fromStdString(filePath.extension().string()).toLower();

  if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
      extension == ".gif" || extension == ".bmp" || extension == ".webp") {
    QPixmap pixmap(QString::fromStdString(filePath.string()));
    if (pixmap.isNull()) {
      QMessageBox::warning(this, "Error", "Failed to load image");
      return false;
    }
    imageViewer->setPixmap(pixmap.scaled(
        imageViewer->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    contentStack->setCurrentWidget(imageViewer);
    return true;
  }

  else if (extension == ".mp4" || extension == ".avi" || extension == ".mkv" ||
           extension == ".mov" || extension == ".webm") {
    videoPlayer->setSource(
        QUrl::fromLocalFile(QString::fromStdString(filePath.string())));
    videoPlayer->play();
    contentStack->setCurrentWidget(videoWidget);
    return true;
  }

  else {
    try {
      std::ifstream file(filePath, std::ios::binary);
      if (file) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        textViewer->setText(QString::fromStdString(content));
        contentStack->setCurrentWidget(textViewer);
        return true;
      }
    } catch (const std::exception &e) {
      QMessageBox::critical(this, "Error",
                            QString("Failed to read file: %1").arg(e.what()));
      return false;
    }
  }
  return false;
}

SecureViewer::~SecureViewer() {
  cleanupTempFiles();
  if (fs::exists(tempDir)) {
    fs::remove_all(tempDir);
  }
}

fs::path SecureViewer::createSecureTempDir() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 35);
  const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";

  std::string random_str = "secview_";
  for (int i = 0; i < 12; i++) {
    random_str += charset[dis(gen)];
  }

  auto temp_path = fs::temp_directory_path() / random_str;
  fs::create_directory(temp_path);
  return temp_path;
}

bool SecureViewer::decryptFile(const fs::path &encryptedFile,
                               const QString &password) {
  if (!fs::exists(encryptedFile)) {
    QMessageBox::critical(this, "Error", "File not found!");
    return false;
  }

  fs::path tempDecryptDir = createSecureTempDir();
  fs::permissions(tempDecryptDir, fs::perms::owner_read |
                                      fs::perms::owner_write |
                                      fs::perms::owner_exec);
  tempFiles.push_back(tempDecryptDir);

  fs::path tempEncFile = tempDecryptDir / encryptedFile.filename();
  fs::copy_file(encryptedFile, tempEncFile);
  fs::permissions(tempEncFile, fs::perms::owner_read | fs::perms::owner_write |
                                   fs::perms::owner_exec);

  std::string cmd = "cd \"" + tempDecryptDir.string() +
                    "\" && "
                    "echo \"" +
                    escapeShellArg(password.toStdString()) +
                    "\" | "
                    "./\"" +
                    tempEncFile.filename().string() + "\" 2>&1";

  std::string output;
  bool success = execCommand(cmd, output);

  if (!success) {
    QMessageBox::critical(
        this, "Error",
        QString("Decryption failed:\n%1").arg(QString::fromStdString(output)));
    return false;
  }

  bool foundDecrypted = false;
  for (const auto &entry : fs::directory_iterator(tempDecryptDir)) {
    if (entry.is_regular_file() && entry.path().extension() != ".senc" &&
        entry.path() != tempEncFile) {
      auto writeTime = fs::last_write_time(entry.path());
      auto now = fs::file_time_type::clock::now();
      if (now - writeTime < std::chrono::seconds(10)) {
        if (displayContent(entry.path())) {
          tempFiles.push_back(entry.path());
          foundDecrypted = true;
          break;
        }
      }
    }
  }

  if (!foundDecrypted) {
    QMessageBox::critical(this, "Error", "No decrypted file found!");
    return false;
  }

  return true;
}

std::string SecureViewer::escapeShellArg(const std::string &arg) {
  std::string escaped;
  escaped.reserve(arg.length() + 4);

  for (char c : arg) {
    switch (c) {
    case '\'':
      escaped += "'\\''";
      break;
    case '\\':
    case '"':
    case '$':
    case '`':
      escaped += '\\';
      [[fallthrough]];
    default:
      escaped += c;
    }
  }

  return escaped;
}

bool SecureViewer::execCommand(const std::string &cmd, std::string &output) {
  std::array<char, 4096> buffer;
  output.clear();

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    output = "Failed to execute command";
    return false;
  }

  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    output += buffer.data();
  }

  int status = pclose(pipe);
  return status == 0;
}

void SecureViewer::cleanupTempFiles() {
  for (const auto &path : tempFiles) {
    try {
      if (fs::exists(path)) {
        if (fs::is_directory(path)) {
          for (const auto &entry : fs::directory_iterator(path)) {
            if (fs::is_regular_file(entry)) {
              std::ofstream file(entry.path(),
                                 std::ios::binary | std::ios::out);
              if (file) {
                auto size = fs::file_size(entry.path());
                std::vector<char> zeros(4096, 0);
                while (size > 0) {
                  size_t toWrite = std::min(size, zeros.size());
                  file.write(zeros.data(), toWrite);
                  size -= toWrite;
                }
                file.close();
              }
            }
          }
        }
        fs::remove_all(path);
      }
    } catch (const std::exception &e) {
      qWarning() << "Error during cleanup:" << e.what();
    }
  }
  tempFiles.clear();

  textViewer->clear();
  imageViewer->clear();
  videoPlayer->stop();
  passwordInput->clear();
}

void SecureViewer::openFile() {
  QString fileName =
      QFileDialog::getOpenFileName(this, "Open Encrypted File", "",
                                   "Encrypted Files (*.senc);;All Files (*)");

  if (fileName.isEmpty()) {
    return;
  }

  if (passwordInput->text().isEmpty()) {
    QMessageBox::warning(this, "Warning",
                         "Please enter the decryption password!");
    return;
  }

  if (decryptFile(fileName.toStdString(), passwordInput->text())) {
    autoDeleteTimer->start();
    textViewer->setReadOnly(true);
    isEditing = false;
    contentModified = false;
  }
}

void SecureViewer::clearContent() {
  cleanupTempFiles();
  videoPlayer->stop();
  textViewer->clear();
  imageViewer->clear();
  passwordInput->clear();
  autoDeleteTimer->stop();
  textViewer->setReadOnly(false);
  isEditing = true;
  contentModified = false;
  currentFilePath.clear();
}

void SecureViewer::saveAndEncrypt() {
  if (contentModified && contentStack->currentWidget() == textViewer) {
    // Handle text editor content
    QString downloadsPath =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString tempFile = downloadsPath + "/temp_content.txt";
    saveAndEncryptFile(tempFile);
  } else if (!currentFilePath.isEmpty()) {
    // Handle uploaded/dropped file
    saveAndEncryptFile(currentFilePath);
  }
}

// New method to handle the actual encryption
void SecureViewer::saveAndEncryptFile(const QString &filePath) {
  bool ok;
  QString password = QInputDialog::getText(
      this, "Encryption Password",
      "Enter password (minimum 6 characters):", QLineEdit::Password, "", &ok);

  if (!ok || password.length() < 6) {
    QMessageBox::warning(this, "Warning", "Invalid password!");
    return;
  }

  QString verify =
      QInputDialog::getText(this, "Verify Password",
                            "Verify password:", QLineEdit::Password, "", &ok);

  if (!ok || verify != password) {
    QMessageBox::critical(this, "Error", "Passwords do not match!");
    return;
  }

  // Create a temporary file for the password
  QString tempPwdFile =
      QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
      "/temp_pwd_" + QString::number(QCoreApplication::applicationPid());
  QFile pwdFile(tempPwdFile);
  if (pwdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&pwdFile);
    out << password << "\n" << password << "\n";
    pwdFile.close();
  } else {
    QMessageBox::critical(this, "Error",
                          "Failed to create temporary password file!");
    return;
  }

  // Get the directory of the file
  fs::path filedir = fs::path(filePath.toStdString()).parent_path();

  // Execute senc directly on the file in its location
  std::string cmd =
      "cd \"" + filedir.string() + "\" && " + "cat \"" +
      fs::path(tempPwdFile.toStdString()).string() + "\" | " + "senc \"" +
      fs::path(filePath.toStdString()).filename().string() + "\" 2>&1";

  std::string output;
  bool success = execCommand(cmd, output);

  // Clean up temporary password file
  QFile::remove(tempPwdFile);

  if (success) {
    contentModified = false;
    QMessageBox::information(this, "Success", "File encrypted successfully!");
  } else {
    QMessageBox::critical(
        this, "Error",
        QString("Encryption failed:\n%1").arg(QString::fromStdString(output)));
  }
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  SecureViewer viewer;
  viewer.show();
  return app.exec();
}
