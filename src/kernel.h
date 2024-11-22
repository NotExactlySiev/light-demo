#pragma once

#define RCntCNT3    0xF2000003
#define EvSpINT     0x0002
#define EvMdINTR    0x1000

int EnterCriticalSection(void);
void ExitCriticalSection(void);
int OpenEvent(unsigned int,int,int,int (*func)());
int CloseEvent(int);
int WaitEvent(int);
int TestEvent(int);
int EnableEvent(int);
int DisableEvent(int);
int enable_timer_irq(unsigned int);
int disable_timer_irq(unsigned int);

int atoi(const char *);
void printf(const char *, ...);
