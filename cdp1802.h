#pragma once
#include <stdint.h>

extern uint8_t M[0x10000];
extern uint16_t R[0x10];
extern uint8_t D;
extern int DF;
extern uint8_t P;
extern uint8_t X;
extern uint8_t T;
extern int IE;
extern int Q;
extern int EF1;
extern int EF2;
extern int EF3;
extern int EF4;

int step(void);

int inp(int i);
void outp(int i, uint8_t val);
