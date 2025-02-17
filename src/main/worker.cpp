#include <QObject>
#include <cstdio>
#include "worker.h"

void Worker::doHeavyTask()
{
    emit deviceReady(); // TODO: Better raytracing initialization

    qDebug() << "void Worker::doHeavyTask()";

    emit taskFinished();
}

