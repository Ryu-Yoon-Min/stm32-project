#ifndef SRC_FND_CONTROLLER_H_
#define SRC_FND_CONTROLLER_H_

#include "main.h"


void digit4_temper(int n, int replay, uint8_t showZero);

void send(uint8_t X);
void send_port(uint8_t X, uint8_t port);
void digit4_show(int n, int replay, uint8_t showZero);
void digit4_replay(int n, int replay);
void digit4(int n);
void digit4showZero_replay(int n, int replay);
void digit4showZero(int n);
void digit2(int n, int port, int replay);
void digit2_port(int n, int port);

#endif
