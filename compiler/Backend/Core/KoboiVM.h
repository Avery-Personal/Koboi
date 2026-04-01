#ifndef KOBOI_VIRTUAL_MACHINE_H
#define KOBOI_VIRTUAL_MACHINE_H

    #include <stdint.h>

    #include "KVMContext.h"

    typedef struct RuntimeVM RuntimeVM;
    typedef struct CompileTimeVM CompileTimeVM;

    typedef enum {
        KOBOI_EXECUTION_MODE_RUNTIME,
        KOBOI_EXECUTION_MODE_COMPILETIME
    } KoboiExecutionMode;

    typedef struct {
        int DEBUG;

        uint32_t RegisterCount;
    } KVMConfiguration;

    typedef struct {
        KoboiExecutionMode Mode;
        KVMConfiguration Configuration;

        KVMContext *context;

        RuntimeVM *RuntimeVM;
        CompileTimeVM *CompileTimeVM;
    } KoboiVM;

#endif
