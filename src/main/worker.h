#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include "vulkanrenderer.h"

class Worker : public QObject
{
    Q_OBJECT
public slots:
    void doHeavyTask();

signals:
    void taskFinished();

    void deviceReady();

};

#endif // WORKER_H
