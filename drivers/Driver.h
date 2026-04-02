#ifndef DRIVER_H
#define DRIVER_H

    #include <stdint.h>

    typedef struct KoboiVM KoboiVM;

    typedef enum {
        KDRIVER_OK = 0,
        KDRIVER_ERR = -1
    } KDriverResult;

    typedef struct KoboiDriver {
        const char *Name;

        KDriverResult (*Initialize)(KoboiDriverContext *Context);
        void (*Shutdown)(KoboiDriverContext *Context);

        void (*update)(KoboiDriverContext *Context);
    } KoboiDriver;

    struct KoboiDriverContext {
        KoboiVM *KVM;

        void *DriverData;
        uint32_t Capabilities;
    };

#endif
