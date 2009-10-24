#ifndef _SETUP_H_
#define _SETUP_H_

#include "slaves.h"

int initialize(void);
void initDefaultConfig(ysm_config_t *cfg);
void initConfig(void);
void initSlaves(void);
void setupHomeDirectory(void);

void readConfig(FILE *fd, char reload);
void readSlaves(FILE *fd);

void addSlave(char *name, uin_t uin);
void addSlaveToDisk(slave_t *victim);
void deleteSlaveFromDisk(uin_t uin);

#endif /* _SETUP_H_*/
