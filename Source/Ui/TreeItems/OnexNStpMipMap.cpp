#include "OnexNStpMipMap.h"
#include "OnexNStpData.h"

OnexNStpMipMap::OnexNStpMipMap(QByteArray header, QString name, QByteArray content, int width, int height, int format,
                               NosZlibOpener *opener, int id, int creationDate, bool compressed)
    : OnexTreeImage(header, name, content, opener, id, creationDate, compressed), width(width), height(height),
      format(format) {
}

QImage OnexNStpMipMap::getImage() {
    ImageResolution resolution = this->getResolution();
    int format = this->getFormat();

    if (format == 0)
        return imageConverter.convertGBAR4444(content, resolution.x, resolution.y);
    else if (format == 1)
        return imageConverter.convertARGB555(content, resolution.x, resolution.y);
    else if (format == 2)
        return imageConverter.convertBGRA8888(content, resolution.x, resolution.y);
    else if (format == 3 || format == 4)
        return imageConverter.convertGrayscale(content, resolution.x, resolution.y);
    else {
        qDebug().noquote().nospace() << "Unknown format! (" << format << ")";
        return QImage(resolution.x, resolution.y, QImage::Format_Invalid);
    }
}

ImageResolution OnexNStpMipMap::getResolution() {
    return ImageResolution{this->width, this->height};
}

FileInfo *OnexNStpMipMap::getInfos() {
    FileInfo *infos = generateInfos();
    connect(this, SIGNAL(replaceInfo(FileInfo *)), infos, SLOT(replace(FileInfo *)));
    return infos;
}

FileInfo *OnexNStpMipMap::generateInfos() {
    FileInfo *infos = OnexTreeImage::generateInfos();

    connect(infos->addIntLineEdit("Format", getFormat()), &QLineEdit::textChanged,
            [=](const QString &value) { setFormat(value.toInt()); });

    return infos;
}

int OnexNStpMipMap::onReplace(QString directory) {
    QString path;
    if (!directory.endsWith(".png"))
        path = directory + this->getName() + ".png";
    else
        path = directory;

    if (!QFile(path).exists()) {
        OnexNStpData *pItem = static_cast<OnexNStpData *>(QTreeWidgetItem::parent());
        path = directory + pItem->getName() + ".png";
    }
    if (!QFile(path).exists()) {
        QMessageBox::critical(NULL, "Woops", "Missing " + path);
        return 0;
    }

    QImage image = importQImageFromSelectedUserFile(path);
    image = image.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (image.isNull() && this->getResolution().x != 0 && this->getResolution().y != 0) {
        QMessageBox::critical(NULL, "Woops", "Couldn't read image " + path);
        return 0;
    }

    if (!hasGoodResolution(image.width(), image.height())) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            0, "Resolution changed",
            "The resolution of the image " + name + " doesn't match!\nDo you want to replace it anyway?");
        if (reply == QMessageBox::No)
            return 0;
    }

    int format = this->getFormat();

    if (format < 0 || format > 4) {
        QMessageBox::critical(NULL, "Woops", "Format of " + path + "is not supported!");
        return 0;
    }

    QByteArray newContent;
    if (format == 0)
        newContent.push_back(imageConverter.toGBAR4444(image));
    else if (format == 1)
        newContent.push_back(imageConverter.toARGB555(image));
    else if (format == 2)
        newContent.push_back(imageConverter.toBGRA8888(image));
    else if (format == 3 || format == 4)
        newContent.push_back(imageConverter.toGrayscale(image));

    content = newContent;
    setWidth(image.width());
    setHeight(image.height());

    emit OnexTreeImage::replaceSignal(this->getImage());

    return 1;
}

int OnexNStpMipMap::getWidth() {
    return width;
}
int OnexNStpMipMap::getHeight() {
    return height;
}
int OnexNStpMipMap::getFormat() {
    return format;
}
QByteArray OnexNStpMipMap::getContent() {
    return content;
}

void OnexNStpMipMap::setWidth(int width, bool update) {
    this->width = width;
    if (update)
        emit changeSignal("Width", width);
}

void OnexNStpMipMap::setHeight(int height, bool update) {
    this->height = height;
    if (update)
        emit changeSignal("Height", height);
}

void OnexNStpMipMap::setFormat(uint8_t format, bool update) {
    this->format = format;
    if (update)
        emit changeSignal("Format", format);
}

OnexNStpMipMap::~OnexNStpMipMap() {
}
