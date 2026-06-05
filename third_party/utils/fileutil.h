/**
 * 文件操作工具
 */

#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

QString readTextFile(QString path, QString codec = QString());
QString readTextFileAutoCodec(QString path, QString *usedCodec = nullptr); // 自动判断GBK或UTF-8
QString readExistedTextFile(QString path); // 文件不存在则创建
QString readTextFileIfExist(QString path); // 如果文件不存在，则不管，只返回空字符串
bool writeTextFile(QString path, const QString &text, QString codec = QString());

QString readTextFileWithFolder(QString file, QString folder, QString codec = "UTF-8");

bool copyFile(QString old_path, QString newPath, bool cover = false); // 复制文件
bool copyFile2(QString old_path, QString new_path); // 复制文件（覆盖）

bool deleteFile(QString path);
bool renameFile(QString path, QString new_path/*完整路径*/, bool override = false);
bool renameDir(QString path, QString new_path, bool override = false);
bool recycleFile(const QString &filename);

bool ensureFileExist(QString path); // 确保文件存在，不存在则创建
bool ensureDirExist(QString path); // 确保目录存在，不存在则创建
bool ensureFileNotExist(QString path); // 确保文件不存在，存在则删除
bool isFileExist(QString path);
bool isDir(QString path); // 是文件夹（并且存在）
QString getPathWithIndex(QString dir, QString name, QString suffix); // 避免文件覆盖
QString getDirByFile(QString file_path);
bool canBeFileName(QString name);
void addLinkToDeskTop(const QString &filename,const QString &name);

bool copyDir(const QString &source, const QString &destination, bool override = false);
bool deleteDir(const QString &path);

#endif // FILEUTIL_H
