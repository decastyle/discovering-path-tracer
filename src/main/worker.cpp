#include <QObject>
#include <cstdio>
#include "worker.h"

void Worker::doHeavyTask() {
    // Perform heavy computation here
    for (int i = 0; i < 1000000; ++i) {
        printf("Iteration: %d\n", i);
    }
    emit taskFinished();
}

