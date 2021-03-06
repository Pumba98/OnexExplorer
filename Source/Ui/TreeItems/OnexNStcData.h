#ifndef ONEXNSTCDATA_H
#define ONEXNSTCDATA_H

#include "OnexTreeImage.h"

class OnexNStcData : public OnexTreeImage {
Q_OBJECT
public:
    OnexNStcData(QString name, QByteArray content, NosZlibOpener *opener, int id = -1,
                 int creationDate = 0, bool compressed = false);
    OnexNStcData(QJsonObject jo, NosZlibOpener *opener, const QString &directory);
    ~OnexNStcData() override;
    QImage getImage() override;
    ImageResolution getResolution() override;
public slots:
    int afterReplace(QImage image) override;
    void setWidth(int width, bool update = false);
    void setHeight(int height, bool update = false);
protected:
    bool hasGoodResolution(int x, int y) override;
};

#endif // ONEXNSTCDATA_H
