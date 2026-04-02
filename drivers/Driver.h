#ifndef DRIVER_H
#define DRIVER_H

    #include <stdint.h>

    #include "Core/DriverRegistry.h"
    #include "Core/DriverCapabilities.h"

    typedef struct KoboiVM KoboiVM;

    typedef enum {
        KDRIVER_OK = 0,
        KDRIVER_ERR = -1
    } KDriverResult;

    typedef struct {
        KoboiVM *KVM;

        void *DriverData;
        uint32_t Capabilities;
    } KoboiDriverContext;

    typedef struct KoboiDriver {
        const char *Name;

        KDriverResult (*Initialize)(KoboiDriverContext *Context);
        void (*Shutdown)(KoboiDriverContext *Context);

        void (*Update)(KoboiDriverContext *Context);
    } KoboiDriver;

#endif
