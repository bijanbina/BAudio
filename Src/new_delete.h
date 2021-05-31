// NewDelete.cpp -  CPP placement new and delete operators implementation

PVOID operator new(size_t iSize, _When_((poolType& NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))    POOL_TYPE       poolType);

PVOID operator new(size_t iSize, _When_((poolType& NonPagedPoolMustSucceed) != 0,
    __drv_reportError("Must succeed pool allocations are forbidden. "
        "Allocation failures cause a system crash")) POOL_TYPE poolType, ULONG tag);

void __cdecl operator delete(PVOID pVoid);
void __cdecl operator delete(PVOID pVoid, ULONG tag);
void __cdecl operator delete(_Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid, _In_ size_t cbSize);

void __cdecl operator delete[](_Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid);
void __cdecl operator delete[](_Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid, _In_ size_t cbSize);
