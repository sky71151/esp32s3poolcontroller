#ifndef STREAMS_H
#define STREAMS_H

#include "main.h"

void setupStreamInputs();
void firmwareCallback(FirebaseStream data);
void firmwareTimeout(bool timeout);
void streamCallback(FirebaseStream data);
void streamTimeout(bool timeout);
#endif
