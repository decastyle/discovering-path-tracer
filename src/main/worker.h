#ifndef WORKER_H
#define WORKER_H

#include <QObject>

class Worker : public QObject
{
    Q_OBJECT

public slots:
    void doHeavyTask();

signals:
    void taskFinished();
};

#endif // WORKER_H
