#include <stdio.h>
#include <stdlib.h>

#include "FilesystemDriver.h"

static KDriverResult FilesystemInitialize(KoboiDriverContext *Context) {
    Context -> Capabilities = KDRIVER_CAP_FS;
    
    return KDRIVER_OK;
}

static void FilesystemShutdown(KoboiDriverContext *Context) {
    (void)Context;
}

static KDriverResult FilesystemOpen(KoboiDriverContext *Context, const char *Path, KFile *Out) {
    FILE *KFILE = fopen(Path, "rb");
    if (!KFILE)
        return KDRIVER_ERR;

    Out -> Handle = KFILE;

    return KDRIVER_OK;
}

static KDriverResult FilesystemRead(KoboiDriverContext *Context, KFile *File, void *Buffer, size_t Size, size_t *ReadBytes) {
    FILE *KFILE = (FILE *) File -> Handle;

    *ReadBytes = fread(Buffer, 1, Size, KFILE);

    return KDRIVER_OK;
}

static void FilesystemClose(KoboiDriverContext *Context, KFile *File) {
    FILE *KFILE = (FILE *) File -> Handle;

    fclose(KFILE);
}

static KoboiFSDriver POSIXFilesystemDriver = {
    .Base = {
        .Name = "POSIXFilesystem",

        .Initialize = FilesystemInitialize,
        .Shutdown = FilesystemShutdown,
        .Update = NULL
    },

    .Open = FilesystemOpen,
    .Read = FilesystemRead,
    .Write = NULL,
    .Close = FilesystemClose
};
