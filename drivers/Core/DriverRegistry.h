#ifndef KOBOI_DRIVER_REGISTRY_H
#define KOBOI_DRIVER_REGISTRY_H

    #include <stdlib.h>

    #define MAX_DRIVERS 32

    typedef struct KoboiDriver KoboiDriver;

    typedef struct {
        KoboiDriver *Drivers[MAX_DRIVERS];

        size_t Count;
    } KoboiDriverRegistry;

    void KDriverRegistryInitialize(KoboiDriverRegistry *Register);
    int KDriverRegister(KoboiDriverRegistry *Register, KoboiDriver *Driver);
    KoboiDriver *KDriverFind(KoboiDriverRegistry *Register, const char *Name);

#endif