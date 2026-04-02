#ifndef COMPILETIME_KVM_H
#define COMPILETIME_KVM_H

    typedef struct {

    } CompileTimeKVM;
    
    CompileTimeKVM *CompileTimeKVMCreate();
    void CompileTimeKVMDestroy(CompileTimeKVM *KVM);

#endif
