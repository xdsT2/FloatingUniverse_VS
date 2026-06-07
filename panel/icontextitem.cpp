#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QToolTip>
#include <QFileIconProvider>
#include <QDesktopServices>
#include <QInputDialog>
#include <QFileDialog>
#include <QProcess>
#include <QSettings>
#include <QDrag>
#include <QMimeData>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include "icontextitem.h"
#include "universepanel.h"
#include "runtime.h"
#include "usettings.h"
#include "facilemenu.h"
#include "fileutil.h"
#include "textinputdialog.h"

IconTextItem::IconTextItem(QWidget *parent) : PanelItemBase(parent)
{
    iconLabel = new QLabel(this);
    textLabel = new MarqueeLabel(this);
    iconLabel->setObjectName("IconLabel");
    textLabel->setObjectName("TextLabel");

    applyIconSize();

    // 当 settings 改变时动态更新图标尺寸
    connect(us, &USettings::panelIconSizeChanged, this, &IconTextItem::applyIconSize);

    // 垂直布局：图标在上，名字在下
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);
    layout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    layout->addWidget(textLabel, 0, Qt::AlignHCenter);

    updateTextColor();

    type = PanelItemType::IconText;
}

MyJson IconTextItem::toJson() const
{
    MyJson json = PanelItemBase::toJson();

    json.insert("icon", iconName);
    json.insert("text", textLabel->text());
    json.insert("link", link);
    json.insert("hide_after_trigger", hideAfterTrigger);
    json.insert("fast_open", fastOpen);
    json.insert("open_level", openLevel);

    return json;
}

void IconTextItem::fromJson(const MyJson &json)
{
    PanelItemBase::fromJson(json);

    // 基础数据
    QString iconName = json.s("icon");
    setIcon(iconName);
    setText(json.s("text"));

    // 扩展数据
    link = json.s("link");
    hideAfterTrigger = json.b("hide_after_trigger", hideAfterTrigger);
    fastOpen = json.b("fast_open", fastOpen);
    openLevel = json.i("open_level", openLevel);
}

void IconTextItem::applyIconSize()
{
    int iconSize = us->panelIconSize;
    iconLabel->setFixedSize(iconSize, iconSize);
    iconLabel->setScaledContents(false);
    iconLabel->setAlignment(Qt::AlignCenter);

    textLabel->setFixedWidth(iconSize);
    textLabel->setWordWrap(false);
    textLabel->setAlignment(Qt::AlignCenter);

    int widgetW = iconSize + 8;
    setFixedWidth(widgetW);

    // 刷新图标显示
    if (!iconName.isEmpty()) {
        setIcon(iconName);
    }
}

void IconTextItem::initResource()
{
    setFastOpen(us->fastOpenDir);
    setOpenDirLevel(us->fastOpenDirLevel);

    // 检查本地文件是否仍然存在
    if (type == LocalFile) {
        QStringList realLinks = getRealLinks();
        for (const auto& realLink : realLinks) {
            if (!realLink.startsWith(FILE_PREFIX) && !QFileInfo(realLink).exists()) {
                fileMissing = true;
                textLabel->setFullText(text);
                updateTextColor();
                qWarning() << "[IconTextItem] 文件已不存在:" << realLink;
                break;
            }
        }
    }
}

void IconTextItem::updateTextColor()
{
    bool isDark = isDarkTheme();

    // 确保 label 背景透明
    textLabel->setAttribute(Qt::WA_TranslucentBackground, true);

    if (fileMissing) {
        // 文件丢失时显示灰色半透明
        textLabel->setStyleSheet("#TextLabel { color: rgba(128, 128, 128, 160); background: transparent; }");
    }
    else if (isDark) {
        textLabel->setStyleSheet("#TextLabel { color: #E0E0E0; background: transparent; }");
    } else {
        // 浅色主题：深色文字 + 半透明阴影底增强可读性
        textLabel->setStyleSheet("#TextLabel { color: #222222; background: transparent; }");
    }
}

/// 释放所有的资源
/// 删除item的时候必须执行，否则变为垃圾文件
void IconTextItem::releaseResource()
{
    PanelItemBase::releaseResource();

    if (!iconName.isEmpty())
    {
        deleteFile(rt->ICON_PATH + iconName);
    }

    // 删除临时生成的文件
    QStringList realLinks = getRealLinks();
    for (auto realLink: realLinks)
    {
        if (realLink.startsWith(rt->PANEL_FILE_PATH) && isFileExist(realLink))
        {
            qInfo() << "删除文件/目录：" << realLink;
            if (!recycleFile(realLink))
            {
                qWarning() << "    放入回收站失败，执行强制删除(不可恢复)";
                // 删除失败，执行强制删除
                deleteFile(realLink);
            }
        }
    }
}

void IconTextItem::setIcon(const QString &iconName)
{
    this->iconName = iconName;
    if (iconName.isEmpty())
    {
        iconLabel->hide();
        return ;
    }
    else
    {
        iconLabel->show();
    }

    QIcon icon(iconName.startsWith(":") ? iconName : rt->ICON_PATH + iconName);
    if (!icon.isNull())
    {
        QPixmap pixmap = icon.pixmap(us->panelIconSize, us->panelIconSize);
        // 保持纵横比缩放，居中显示
        pixmap = pixmap.scaled(us->panelIconSize, us->panelIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        iconLabel->setPixmap(pixmap);
    }
}

void IconTextItem::setText(const QString &text)
{
    this->text = text;
    // 使用MarqueeLabel的setFullText，自动判断是否需要滚动
    textLabel->setFullText(text);
    if (text.isEmpty())
        textLabel->hide();
    else
        textLabel->show();
}

void IconTextItem::setLink(const QString &link)
{
    this->link = link;

    // 判断是不是快捷方式
    QFileInfo info(link);
    if (info.isFile() && info.exists() && info.isSymLink())
    {
        // 如果是快捷方式，则直接指向链接的路径
        this->link = info.symLinkTarget();
        qInfo() << " 快捷方式原路径：" << this->link;
    }

    setToolTip(this->link);
}

void IconTextItem::setFastOpen(bool fast)
{
    this->fastOpen = fast;
}

void IconTextItem::setOpenDirLevel(int level)
{
    this->openLevel = level;
}

QString IconTextItem::getText() const
{
    return text;
}

QString IconTextItem::getIconName() const
{
    return iconName;
}

QString IconTextItem::getLink() const
{
    return link;
}

QStringList IconTextItem::getRealLinks() const
{
    QString realLink = link;
    realLink.replace(FILE_PREFIX, rt->PANEL_FILE_PATH);
    return realLink.split("\n", Qt::SkipEmptyParts);
}

bool IconTextItem::isFastOpen() const
{
    return fastOpen;
}

QString IconTextItem::saveIconFile(const QIcon &icon)
{
    return saveIconFile(icon.pixmap(us->panelIconSize, us->panelIconSize));
}

QString IconTextItem::saveIconFile(const QPixmap &pixmap)
{
    // 保存到本地
    int val = 1;
    while (QFileInfo(rt->ICON_PATH + QString::number(val) + ".png").exists())
        val++;
    QString iconName = QString::number(val) + ".png";

    pixmap.save(rt->ICON_PATH + iconName);
    return iconName;
}

/// 左右震动
/// 拒绝 drop
void IconTextItem::shake(int range)
{
    if (_shaking)
        return ;

    int nX = this->x();
    int nY = this->y();
    QPropertyAnimation *ani = new QPropertyAnimation(this, "geometry");
    ani->setEasingCurve(QEasingCurve::InOutSine);
    ani->setDuration(300);
    ani->setStartValue(QRect(QPoint(nX,nY), this->size()));

    int nShakeCount = 20; //抖动次数
    double nStep = 1.0 / nShakeCount;
    for(int i = 1; i < nShakeCount; i++){
        range = i & 1 ? -range : range;
        ani->setKeyValueAt(nStep * i, QRect(QPoint(nX + range, nY), this->size()));
    }

    ani->setEndValue(QRect(QPoint(nX,nY), this->size()));
    ani->start(QAbstractAnimation::DeleteWhenStopped);

    _shaking = true;
    connect(ani, &QPropertyAnimation::stateChanged, this, [=](QAbstractAnimation::State state){
        if (state == QPropertyAnimation::State::Stopped)
        {
            _shaking = false;
        }
    });
}

/// 整体点头
/// 单个 menu
void IconTextItem::nod(int range)
{
    int nX = this->x();
    int nY = this->y();
    QPropertyAnimation *ani = new QPropertyAnimation(this, "geometry");
    ani->setEasingCurve(QEasingCurve::InOutSine);
    ani->setDuration(300);
    ani->setStartValue(QRect(QPoint(nX,nY), this->size()));

    int nShakeCount = 3;
    double nStep = 1.0 / nShakeCount;
    for(int i = 1; i < nShakeCount; i++){
        range = i & 1 ? -range : range;
        ani->setKeyValueAt(nStep * i, QRect(QPoint(nX, nY + range), this->size()));
    }

    ani->setEndValue(QRect(QPoint(nX,nY), this->size()));
    ani->start(QAbstractAnimation::DeleteWhenStopped);

    _nodding = true;
    connect(ani, &QPropertyAnimation::stateChanged, this, [=](QAbstractAnimation::State state){
        if (state == QPropertyAnimation::State::Stopped)
        {
            _nodding = false;
        }
    });
}

/// 图标跳跃
/// trigger
void IconTextItem::jump(int range)
{
    QWidget* w = !iconLabel->pixmap().isNull() ? static_cast<QWidget*>(iconLabel) : this;
    int nX = w->x();
    int nY = w->y();
    QPropertyAnimation *ani = new QPropertyAnimation(w, "geometry");
    ani->setEasingCurve(QEasingCurve::InOutSine);
    ani->setDuration(500);
    ani->setStartValue(QRect(QPoint(nX,nY), w->size()));

    int nShakeCount = 5;
    double nStep = 1.0 / nShakeCount;
    for(int i = 1; i < nShakeCount; i++){
        range = i & 1 ? -range : range;
        ani->setKeyValueAt(nStep * i, QRect(QPoint(nX, nY + range), w->size()));
    }

    ani->setEndValue(QRect(QPoint(nX,nY), w->size()));
    ani->start(QAbstractAnimation::DeleteWhenStopped);

    _jumping = true;
    connect(ani, &QPropertyAnimation::stateChanged, this, [=](QAbstractAnimation::State state){
        if (state == QPropertyAnimation::State::Stopped)
        {
            _jumping = false;
        }
    });
}

/// 整体缩放
/// drop 修改属性
void IconTextItem::shrink(int range)
{
    int nX = this->x();
    int nY = this->y();
    QPropertyAnimation *ani = new QPropertyAnimation(this, "geometry");
    ani->setEasingCurve(QEasingCurve::InOutSine);
    ani->setDuration(300);
    ani->setStartValue(QRect(QPoint(nX,nY), this->size()));

    int nShakeCount = 3;
    double nStep = 1.0 / nShakeCount;
    for(int i = 1; i < nShakeCount; i++){
        range = i & 1 ? -range : range;
        ani->setKeyValueAt(nStep * i, QRect(nX - range / 2, nY - range / 2, width() + range, height() + range));
    }

    ani->setEndValue(QRect(QPoint(nX,nY), this->size()));
    ani->start(QAbstractAnimation::DeleteWhenStopped);

    layout()->setEnabled(false);
    _shrinking = true;
    connect(ani, &QPropertyAnimation::stateChanged, this, [=](QAbstractAnimation::State state){
        if (state == QPropertyAnimation::State::Stopped)
        {
            layout()->setEnabled(true);
            _shrinking = false;
        }
    });
}

/// 图标抖动
/// hover
/// @param startPos 相对于本控件的鼠标位置，图标向反方向移动
void IconTextItem::jitter(QPoint startPos, int range)
{
    if (_jittering)
        return ;
    if (startPos == UNDEFINED_POS)
        return ;
    if (iconLabel->pixmap().isNull())
        return ;

    QWidget* w = iconLabel;
    int nX = w->x();
    int nY = w->y();
    QPoint effectPos = startPos - rect().center();
    if (effectPos == QPoint(0, 0))
        return ;
    double xie = sqrt(effectPos.x() * effectPos.x() + effectPos.y() * effectPos.y());
    double dx = effectPos.x() / xie; // 最长边为1的直角三角形的x
    double dy = effectPos.y() / xie; // 同上的y

    QPropertyAnimation *ani = new QPropertyAnimation(iconLabel, "geometry");
    ani->setEasingCurve(QEasingCurve::InOutSine);
    ani->setDuration(300);
    ani->setStartValue(QRect(QPoint(nX,nY), w->size()));

    int nShakeCount = 3;
    double nStep = 1.0 / nShakeCount;
    for(int i = 1; i < nShakeCount; i++){
        range = i & 1 ? -range : range;
        ani->setKeyValueAt(nStep * i, QRect(nX - dx * range, nY - dy * range, w->width(), w->height()));
    }
    ani->setEndValue(QRect(QPoint(nX,nY), w->size()));
    ani->start(QAbstractAnimation::DeleteWhenStopped);

    // 避免重复动画
    _jittering = true;
    this->layout()->setEnabled(false);
    connect(ani, &QPropertyAnimation::stateChanged, this, [=](QAbstractAnimation::State state){
        if (state == QPropertyAnimation::State::Stopped)
        {
            _jittering = false;
            this->layout()->setEnabled(true);
        }
    });
}

void IconTextItem::facileMenuEvent(FacileMenu *menu)
{
    nod();

    menu->addAction(QIcon(":/icons/open"), "打开 (&O)", [=]{
        triggerEvent();
    });

    if (type == LocalFile)
    {
        QFileInfo info(link);
        if (info.exists())
        {
            menu->addAction(QIcon(":/icons/folder"), "打开文件夹", [=]{
                QDesktopServices::openUrl("file:///" + info.dir().path());
                if (hideAfterTrigger)
                    emit hidePanel();
            });
        }
    }

    menu->split()->addAction(QIcon(":/icons/rename"), "重命名 (&N)", [=]{
        menu->close();
        bool ok = false;
        QString oldName = text;
        emit keepPanelFixing();
        QString newName = QInputDialog::getText(this, "修改名字", "请输入新的名字", QLineEdit::Normal, text, &ok);
        emit restorePanelFixing();
        if (!ok)
            return ;
        this->setText(newName);
        adjustSize();

        // 询问是否修改文件名
        QStringList realLinks = getRealLinks();
        QStringList newLinks;
        for (auto realLink: realLinks)
        {
            QString newLink = realLink;
            if (us->modifyFileNameSync && isFileExist(realLink))
            {
                QFileInfo info(realLink);
                QString fileName = info.isDir() ? info.fileName() : info.baseName();
                QString suffixName = info.fileName(); // 可能带后缀的强制全文件名
                if (!fileName.isEmpty() && (fileName == oldName || suffixName == oldName) && canBeFileName(newName))
                {
                    int recentRename = us->i("recent/renameFileSync", 0);
                    recentRename = 0; // 强制默认修改，而不是上一次的选项
                    if ((recentRename = QMessageBox::question(this, "重命名", "是否同步修改文件名", "修改", "取消", nullptr, recentRename)) == 0)
                    {
                        if (info.isDir())
                        {
                            QDir dir(realLink);
                            dir.cdUp();
                            newLink = dir.absoluteFilePath(newName);
                            if (!renameDir(realLink, newLink))
                                qWarning() << "重命名失败：" << realLink << "  ->  " <<newLink;
                        }
                        else
                        {
                            QDir dir(info.absoluteDir());
                            bool hasSuffix = (suffixName == oldName);
                            newLink = dir.absoluteFilePath(newName) + (hasSuffix || info.suffix().isEmpty() ? "" : "." + info.suffix());
                            if (!renameFile(realLink, newLink))
                                qWarning() << "重命名失败：" << realLink << "  ->  " <<newLink;
                        }
                        newLink.replace(rt->PANEL_FILE_PATH, FILE_PREFIX);
                    }
                    us->set("recent/renameFileSync", recentRename);
                }
            }
            newLinks.append(newLink);
        }
        setLink(newLinks.join("\n"));

        emit modified();
    });

    menu->addAction(QIcon(":/icons/link"), "链接 (&L)", [=]{
        menu->close();
        bool ok = false;
        emit keepPanelFixing();

        QString newLink = TextInputDialog::getText(this, "修改链接", "可以是文件路径、网址，点击项目后立即打开\n多行将按顺序打开\n也可以是命令行，加上前缀：cmd://", link, &ok);
        emit restorePanelFixing();
        if (!ok)
            return ;
        setLink(newLink);
        emit modified();
    });

    menu->split();

    if (type == LocalFile || type == WebUrl)
    {
        menu->addAction(QIcon(":/icons/eye"), "打开后隐藏", [=]{
            hideAfterTrigger = !hideAfterTrigger;
            emit modified();
        })->check(hideAfterTrigger)->tooltip("打开此项目后，自动隐藏悬浮面板");
    }

    if (type == LocalFile)
    {
        QFileInfo info(link);
        bool isDir = info.exists() && info.isDir();

        if (isDir)
        {
            // 这个命令行显示不了
            menu->addAction(QIcon(), "命令行", [=]{
                QProcess p(this);
                p.setProgram("cmd");
                p.setArguments(QStringList{"/c", "cd", "/d", link});
                p.start();
                p.waitForStarted();
                p.waitForFinished();
            })->hide();

            menu->addAction(QIcon(":/icons/fast_open"), "快速打开", [=]{
                fastOpen = !fastOpen;
                emit modified();
            })->check(fastOpen);

            /* auto levelMenu = menu->addMenu(QIcon(":/icons/open_level"), "加载层数");
            menu->lastAction()->hide(!fastOpen);
            levelMenu->addNumberedActions("%1层", 1, 11, [=](FacileMenuItem* item){
                item->check(item->getText() == QString::number(openLevel) + "层");
            }, [=](int val){
                openLevel = val;
                emit modified();
            }); */
        }
    }

    auto moreMenu = menu->addMenu(QIcon(":/icons/more"), "更多");

    moreMenu->addAction(QIcon(":/icons/happy"), "更换图标", [=]{
        menu->close();
        emit keepPanelFixing();
        QString prevPath = us->s("recent/iconPath");
        QString path = QFileDialog::getOpenFileName(this, "更换图标", prevPath, tr("Images (*.png *.xpm *.jpg *.jpeg *.gif *.ico)"));
        emit restorePanelFixing();
        if (path.isEmpty())
            return ;
        us->set("recent/iconPath", path);

        // 读取图标并保存
        QPixmap pixmap(path);
        if (pixmap.width() > us->panelIconSize || pixmap.height() > us->panelIconSize)
            pixmap = pixmap.scaled(us->panelIconSize, us->panelIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (!pixmap.save(rt->ICON_PATH + this->iconName))
            qWarning() << "保存新图标失败" << path << "  ->  " << (rt->ICON_PATH + this->iconName);
        iconLabel->setPixmap(pixmap);
        this->adjustSize();
        jump();
    });

    moreMenu->addAction(QIcon(":/icons/update"), "刷新图标", [=]{
        if (!QFileInfo(link).exists())
            return ;
        QIcon icon = QFileIconProvider().icon(QFileInfo(link));
        auto pixmap = icon.pixmap(us->panelIconSize, us->panelIconSize);
        pixmap.save(rt->ICON_PATH + iconName);
        iconLabel->setPixmap(pixmap);
    });
}

void IconTextItem::triggerEvent()
{
    if (!getLink().isEmpty())
    {
        QStringList links = getRealLinks();
        bool dirMenuShowed = false;
        for (auto link: links)
        {
            qInfo() << "触发link：" << link;

            // 打开命令行
            if (link.startsWith("cmd://"))
            {
                QProcess p;
                p.startDetached(link.right(link.length() - 6));
                continue;
            }

            // 打开文件或者文件夹或者网页
            QFileInfo info(link);
            if (info.exists()) // 是文件
            {
                // 文件存在，重置丢失标记
                if (fileMissing) {
                    fileMissing = false;
                    textLabel->setFullText(text);
                    updateTextColor();
                }
                if (info.isDir() && isFastOpen()) // 快速打开文件夹
                {
                    if (!dirMenuShowed)
                        showFacileDir(link, nullptr, 0);
                    dirMenuShowed = true;
                    continue;
                }
                else if (link.endsWith(".exe")) // 打开程序
                {
                    QProcess process;
                    process.startDetached(link, QStringList{}, info.path());
                    link = "";
                }
                else if (link.endsWith(".bat") || link.endsWith(".sh") || link.endsWith(".vbs")) // 直接执行命令
                {
                    QProcess process;
                    process.setWorkingDirectory(info.path());
                    process.startDetached("cmd", {"/c", link});
                    link = "";
                }
                else // 打开文件或者文件夹
                {
                    link = "file:///" +link;
                }
            }
            else // 文件不存在
            {
                fileMissing = true;
                textLabel->setFullText(text);
                updateTextColor();
                
                // 主题适配的缺失文件对话框
                QMessageBox dlg(this);
                dlg.setIcon(QMessageBox::Warning);
                dlg.setWindowTitle(tr("文件丢失"));
                dlg.setText(tr("文件已被移动或删除，是否从面板移除此条目？"));
                dlg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                dlg.setDefaultButton(QMessageBox::Yes);
                
                bool isDark = isDarkTheme();
                if (isDark) {
                    dlg.setStyleSheet(
                        "QMessageBox { background: #2b2b2b; color: #E6E6E6; }"
                        "QMessageBox QLabel { color: #E6E6E6; }"
                        "QMessageBox QPushButton { min-width: 80px; background: #3a3a3a; color: #E6E6E6; border: 1px solid #555; padding: 6px 16px; }"
                        "QMessageBox QPushButton:hover { background: #4a4a4a; }"
                    );
                }
                
                int ret = dlg.exec();
                if (ret == QMessageBox::Yes) {
                    emit deleteMe();
                }
                return;
            }
            if (!link.isEmpty())
                QDesktopServices::openUrl(link);
        }
        us->set("usage/linkOpenCount", ++us->linkOpenCount);

        jump();
        if (hideAfterTrigger)
            emit hidePanel();
    }
}

bool IconTextItem::canDropEvent(const QMimeData *mime)
{
    if ((mime->hasUrls() && mime->urls().size() == 1)
            || (mime->hasText() && !mime->text().contains("\n")))
    {
        jitter(mapFromGlobal(QCursor::pos()), -10);
        return true;
    }
    shake();
    return false;
}

void IconTextItem::dropEvent(QDropEvent *event)
{
    auto mime = event->mimeData();

    // 设置链接
    auto setMyLink = [=](QString link) {
        emit selectMe();
        setLink(link);
        qInfo() << "setLink:" << link;
        shrink();
    };

    if (mime->hasUrls())
    {
        auto urls = mime->urls();
        if (!urls.size())
            return PanelItemBase::dropEvent(event);
        QUrl url = urls.first();
        if (url.isLocalFile())
        {
            setMyLink(url.toLocalFile().replace(rt->PANEL_FILE_PATH, FILE_PREFIX));
            setType(PanelItemType::LocalFile);
        }
        else
        {
            setMyLink(url.url());
            setType(PanelItemType::WebUrl);
        }
    }
    else if (mime->hasText())
    {
        QString text = mime->text();
        if (text.contains("\n"))
        {
            shake();
            return PanelItemBase::dropEvent(event);
        }
        setMyLink(text);
        if (isFileExist(text))
            // text 路径，不自动替换为临时路径
            setType(PanelItemType::LocalFile);
        else
            setType(PanelItemType::WebUrl);
    }
    else
    {
        shake();
        return PanelItemBase::dropEvent(event);
    }
    emit modified();
}

bool IconTextItem::startNativeFileDrag()
{
    // 检查设置是否允许拖出
    if (!us->allowMoveOut) {
        qDebug() << "[startNativeFileDrag] 拖出被设置禁止";
        return false;
    }

    QStringList realLinks = getRealLinks();
    qDebug() << "[startNativeFileDrag] realLinks:" << realLinks;
    if (realLinks.isEmpty()) return false;
    
    QList<QUrl> urls;
    foreach (const QString& link, realLinks) {
        QFileInfo fi(link);
        qDebug() << "[startNativeFileDrag] checking:" << link << "exists:" << fi.exists();
        if (fi.exists()) {
            urls.append(QUrl::fromLocalFile(fi.absoluteFilePath()));
        }
    }
    qDebug() << "[startNativeFileDrag] valid URLs:" << urls.size();
    if (urls.isEmpty()) return false;
    
    // 获取所属UniversePanel，恢复所有选中item到拖拽前位置（消除残影）
    UniversePanel* up = qobject_cast<UniversePanel*>(parentWidget());
    qDebug() << "[startNativeFileDrag] UniversePanel:" << (up != nullptr);
    if (up) {
        // 关键：先恢复所有选中item到拖拽前的原始位置，再启动QDrag
        // 这样外部QDrag复制文件后，面板内item保持原位不动
        up->restoreAllToOriginalPositions();
        up->unselectAll();
    }
    
    // 启动原生QDrag
    QDrag* drag = new QDrag(this);
    QMimeData* mime = new QMimeData();
    mime->setUrls(urls);
    // 添加内部标识，防止自身面板作为拖放目标重复创建
    mime->setData("application/x-floating-universe-internal", QByteArray("1"));
    drag->setMimeData(mime);
    qDebug() << "[startNativeFileDrag] starting QDrag with" << urls.size() << "files";
    Qt::DropAction action = drag->exec(Qt::CopyAction);
    qDebug() << "[startNativeFileDrag] QDrag finished with action:" << action;
    return true;
}

void IconTextItem::hoverEvent(const QPoint &startPos)
{
    PanelItemBase::hoverEvent(startPos);

    if (!hovered)
    {
        jitter(startPos);
    }
}

void IconTextItem::enterEvent(QEnterEvent *event)
{
    if (!isSelected() && !isHovered())
    {
        jitter(mapFromGlobal(QCursor::pos()));
    }
    PanelItemBase::enterEvent(event);
}

void IconTextItem::showFacileDir(QString path, FacileMenu *parentMenu, int level)
{
    if (level >= openLevel)
        return ;

    auto connectDynamicMenu = [=](FacileMenu* menu) {
        connect(menu, &FacileMenu::signalDynamicMenuTriggered, this, [=](FacileMenuItem* item) {
            QString path = item->getData().toString();
            if (path.isEmpty())
            {
                qWarning() << "无法获取到目录路径" << path;
                return;
            }
            qInfo() << "动态加载目录列表：" << path;
            showFacileDir(path, item->subMenu(), -1);
        });
    };

    FacileMenu* menu = parentMenu;
    if (!parentMenu)
    {
        menu = new FacileMenu(this);
        connectDynamicMenu(menu);
    }

    auto infos = QDir(path).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileIconProvider provicer;
    int count = 0;
    foreach (auto info, infos)
    {
        if (++count > us->fastOpenDirFileCount)
        {
            menu->addTitle("总计文件(夹)数量：" + QString::number(infos.count()));
            break;
        }

        if (info.isDir())
        {
            auto m = menu->addMenu(provicer.icon(info), info.fileName(), [=]{
                menu->toClose(); // 先关闭菜单，得以隐藏面板；否则即使隐藏也会重新触发enter事件
                QDesktopServices::openUrl("file:///" + info.absoluteFilePath());
                if (hideAfterTrigger)
                {
                    QTimer::singleShot(0, [=]{
                        emit hidePanel();
                    });
                }
            });
            // showFacileDir(info.absoluteFilePath(), m, level+1);
            menu->lastAddedItem()->setDynamicCreate(true)->setData(QString(info.absoluteFilePath()));
            connectDynamicMenu(m);
        }
        else
        {
            menu->addAction(provicer.icon(info), info.fileName(), [=]{
                menu->toClose(); // 先关闭菜单，得以隐藏面板；否则即使隐藏也会重新触发enter事件
                QDesktopServices::openUrl("file:///" + info.absoluteFilePath());
                if (hideAfterTrigger)
                {
                    QTimer::singleShot(0, [=]{
                        emit hidePanel();
                    });
                }
            });
        }
    }

    if (!parentMenu)
    {
        emit facileMenuUsed(menu);

        if (level != -1)
            menu->exec();
    }
}

void IconTextItem::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    bool dark = isDarkTheme();
    
    // 背景色
    QColor bg = dark ? QColor(30, 30, 30, 200) : QColor(250, 250, 250, 200);
    
    // 圆角背景
    QRectF r = rect().adjusted(2, 2, -2, -2);
    QPainterPath path;
    path.addRoundedRect(r, 8, 8);
    
    // 浅色主题下先绘制阴影
    if (!dark) {
        QColor shadow = QColor(0, 0, 0, 50);
        QPainterPath shadowPath;
        shadowPath.addRoundedRect(r.translated(0, 3), 8, 8);
        p.fillPath(shadowPath, shadow);
    }
    
    p.fillPath(path, bg);
    
    // 选中态：发光边框
    if (isSelected()) {
        QColor glow = dark ? QColor(0, 120, 215, 150) : QColor(0, 100, 200, 150);
        QPen pen(glow);
        pen.setWidth(2);
        p.setPen(pen);
        p.drawPath(path);
        
        // 外发光
        QColor outerGlow = glow;
        outerGlow.setAlpha(60);
        p.setPen(Qt::NoPen);
        p.setBrush(outerGlow);
        p.drawRoundedRect(r.adjusted(-2, -2, 2, 2), 10, 10);
    }
    
    // 文件丢失：半透明遮罩
    if (fileMissing) {
        p.fillPath(path, QColor(0, 0, 0, 40));
    }
    
    // 调用父类绘制子控件
    QWidget::paintEvent(ev);
}

// ===== MarqueeLabel 实现 =====
// 默认不滚动（超长时省略显示），鼠标悬停时才启动滚动 + tooltip
MarqueeLabel::MarqueeLabel(QWidget *parent) : QLabel(parent)
{
    setAttribute(Qt::WA_Hover);
    m_timer = new QTimer(this);
    m_timer->setInterval(30); // ~33 FPS
    connect(m_timer, &QTimer::timeout, this, [this]() {
        int pxPerTick = qMax(1, m_speedPxPerSec * m_timer->interval() / 1000);
        m_offset += pxPerTick;
        int textW = fontMetrics().horizontalAdvance(text());
        if (m_offset > textW + 40)
            m_offset = 0;
        update();
    });
}

void MarqueeLabel::setFullText(const QString &txt)
{
    setText(txt);
    // 同时设置tooltip显示完整内容
    setToolTip(txt);
}

void MarqueeLabel::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 使用主题颜色，而不是 palette().windowText()（可能被父控件覆盖）
    bool isDark = isDarkTheme();
    QColor textColor = isDark ? QColor(0xE0, 0xE0, 0xE0) : QColor(0x22, 0x22, 0x22);

    QString txt = text();
    QFontMetrics fm(font());
    int textW = fm.horizontalAdvance(txt);

    // 不需要滚动 或 不在悬停中 → 单行省略显示
    if (!m_scrollingNeeded || !m_isHovering || !m_timer->isActive()) {
        QString elided = fm.elidedText(txt, Qt::ElideRight, width());

        // 浅色主题下绘制半透明阴影底增强可读性
        if (!isDark) {
            p.setPen(Qt::NoPen);
            QColor shadow = QColor(0, 0, 0, 30);
            QRectF bgRect = rect().adjusted(1, 1, -1, -1);
            p.setBrush(shadow);
            p.drawRoundedRect(bgRect.translated(0, 1), 3, 3);
        }

        p.setPen(textColor);
        p.setFont(font());
        QRect r = rect();
        p.drawText(r, alignment(), elided);
        return;
    }

    // 滚动中：绘制两份文字实现无缝循环
    int gap = 40;
    int x = -m_offset;
    int y = (height() + fm.ascent() - fm.descent()) / 2;

    // 浅色主题下绘制阴影底
    if (!isDark) {
        p.setPen(Qt::NoPen);
        QColor shadow = QColor(0, 0, 0, 30);
        QRectF bgRect = rect().adjusted(1, 1, -1, -1);
        p.setBrush(shadow);
        p.drawRoundedRect(bgRect.translated(0, 1), 3, 3);
    }

    p.setPen(textColor);
    p.setFont(font());
    while (x < width()) {
        p.drawText(x, y, txt);
        x += textW + gap;
    }
}

void MarqueeLabel::enterEvent(QEnterEvent* event)
{
    m_isHovering = true;
    updateScrollingNeed();
    if (m_scrollingNeeded) {
        QToolTip::showText(cursor().pos(), text(), this);
        m_timer->start();
    }
    QLabel::enterEvent(event);
}

void MarqueeLabel::leaveEvent(QEvent* event)
{
    m_isHovering = false;
    if (m_timer->isActive()) m_timer->stop();
    m_offset = 0;
    QToolTip::hideText();
    update();
    QLabel::leaveEvent(event);
}

void MarqueeLabel::resizeEvent(QResizeEvent* event)
{
    QLabel::resizeEvent(event);
    updateScrollingNeed();
}

void MarqueeLabel::updateScrollingNeed()
{
    QFontMetrics fm(font());
    m_scrollingNeeded = (fm.horizontalAdvance(text()) > width());
    if (!m_scrollingNeeded && m_timer->isActive()) {
        m_timer->stop();
        m_offset = 0;
        update();
    }
}
