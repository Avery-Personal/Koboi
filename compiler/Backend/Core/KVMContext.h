#ifndef KVM_CONTEXT_H
#define KVM_CONTEXT_H
    
    typedef enum {
        KOBOI_POLICY_SAFE,
        KOBOI_POLICY_CHECKINGS,
        KOBOI_POLICY_UNSAFE,    
        KOBOI_POLICY_TRUSTED_UNSAFE
    } KoboiSafetyMode;

    typedef struct {
        KoboiSafetyMode SafetyMode;

        void *CurrentWorld;
        void *ModuleRegistry;
    } KVMContext;

    KVMContext *KVMContextCreate(void);
    void KVMContextDestroy(KVMContext *Context);

#endif
