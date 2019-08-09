#ifndef ONEXNS4BBDATA_H
#define ONEXNS4BBDATA_H
#include "OnexTreeImage.h"

class OnexNS4BbData : public OnexTreeImage {
    Q_OBJECT
public:
    OnexNS4BbData(QByteArray header, QString name, QByteArray content, NosZlibOpener *opener, int id, int creationDate, bool compressed);

    virtual QImage getImage() override;
    virtual ImageResolution getResolution() override;
    virtual ~OnexNS4BbData();

public slots:
    virtual int onReplace(QString directory) override;
    virtual void setWidth(int width, bool update = false);
    virtual void setHeight(int height, bool update = false);
};

#endif // ONEXNS4BBDATA_H