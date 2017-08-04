#pragma once

#include <wdm.h>

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002
#define PTDBG_TRACE_GALILEO				0x00000004

//#define TraceFlags PTDBG_TRACE_ROUTINES | PTDBG_TRACE_GALILEO
#define TraceFlags PTDBG_TRACE_GALILEO

#define PT_DBG_PRINT( _dbgLevel, _string )        \
    (((TraceFlags) & (_dbgLevel)) ?              \
        DbgPrint _string :                        \
        ((int)0))