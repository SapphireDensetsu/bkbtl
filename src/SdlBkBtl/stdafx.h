// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS  // For VC warning C4996: 'sprintf': This function or variable may be unsafe.

// C RunTime Header Files
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#ifdef PSP
#  include <stdlib.h>
#else
#  include <memory.h>
#endif

#include "Common.h"
