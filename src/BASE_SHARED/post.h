#ifndef SDA_BASE_POST_H
#define SDA_BASE_POST_H
#include "../sda_platform.h"

void postInit();
void postMessage(uint8_t *message);
void postError(uint8_t *message);
void postSuccess(uint8_t *message);

#endif
