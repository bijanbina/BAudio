// NewDelete.cpp -  CPP placement new and delete operators implementation

#ifdef __cplusplus
extern "C" {
#include <wdm.h>
}
#else
#include <wdm.h>
#endif

#include "new_delete.h"

#pragma code_seg()

/*****************************************************************************
* ::new()
*****************************************************************************
* New function for creating objects with a specified allocation tag.
*/
PVOID operator new
(
    size_t          iSize,
    _When_((poolType& NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))
    POOL_TYPE       poolType
    )
{
    PVOID result = ExAllocatePoolWithTag(poolType, iSize, 'wNcP');

    if (result)
    {
        RtlZeroMemory(result, iSize);
    }

    return result;
}

/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with a specified allocation tag.
 */
PVOID operator new(size_t iSize,
    _When_((poolType& NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))
    POOL_TYPE       poolType,
    ULONG           tag
    )
{
    PVOID result = ExAllocatePoolWithTag(poolType, iSize, tag);

    if (result)
    {
        RtlZeroMemory(result, iSize);
    }

    return result;
}

// Sized Delete function.
void __cdecl operator delete(
    _Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid,
    _In_ size_t cbSize
    )
{
    UNREFERENCED_PARAMETER(cbSize);

    if (pVoid)
    {
        ExFreePool(pVoid);
    }
}


//* Sized Array Delete function.
void __cdecl operator delete[]
(
    _Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid
    )
{
    if (pVoid)
    {
        ExFreePool(pVoid);
    }
}


// Array Delete function.
void __cdecl operator delete[](_Pre_maybenull_ __drv_freesMem(Mem) PVOID pVoid,
    _In_ size_t cbSize);