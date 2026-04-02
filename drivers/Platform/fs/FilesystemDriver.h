#ifndef KOBOI_FS_DRIVER_H
#define KOBOI_FS_DRIVER_H

    #include "../../Driver.h"

    typedef struct {
        void *Handle;
    } KFile;

    typedef struct KoboiFSDriver {
        KoboiDriver Base;

        KDriverResult (*Open)(KoboiDriverContext *Context, const char *Path, KFile *Output);
        KDriverResult (*Read)(KoboiDriverContext *Context, KFile *File, void *Buffer, size_t Size, size_t *ReadBytes);
        KDriverResult (*Write)(KoboiDriverContext *Context, KFile *File, const void *Buffer, size_t Size);
        void (*Close)(KoboiDriverContext *Context, KFile *File);
    } KoboiFSDriver;

#endif