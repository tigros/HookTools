#ifndef GETHOOKS_H
#define GETHOOKS_H

#include "exttools.h"
#include "resource.h"
#include <stdio.h>
#include "util.h"
#include "snapshot.h"
#include "diff.h"
#include "test.h"
#include <fcntl.h>
#include <io.h>
/* the global stores */
#include "global.h"

extern struct snapshot *glbupdated;
extern struct snapshot *glbcurrent;

int gethooks();
BOOL Isdiffhooks();

#endif