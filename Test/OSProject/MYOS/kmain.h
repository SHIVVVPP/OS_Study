#pragma once
#include "stdint.h"
#include "defines.h"
#include "string.h"
#include "sprintf.h"
#include "MultiBoot.h"
#include "CYNConsole.h"
#include "InitKernel.h"
#include "CYNAPI.h"
#include "GDT.h"
#include "IDT.h"
#include "PIC.h"
#include "PIT.h"


void kmain(unsigned long, unsigned long);
