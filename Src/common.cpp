// Implementation of the AdapterCommon class.

#include "sonicsaudio.h"
#include "common.h"
#include "hw.h"
#include "adapter_common.h"
#include "new_delete.h"
//#include <new>

#pragma code_seg("PAGE")
// Creates a new CAdapterCommon
NTSTATUS NewAdapterCommon(OUT PUNKNOWN *Unknown, IN REFCLSID,
    IN PUNKNOWN UnknownOuter OPTIONAL, IN POOL_TYPE PoolType)
{
	PAGED_CODE();
	ASSERT(Unknown);
    NTSTATUS ntStatus;
    //ULONG64               PoolFlags;
	//STD_CREATE_BODY_(CAdapterCommon, Unknown, UnknownOuter, PoolType, PADAPTERCOMMON);
    //STD_CREATE_BODY_WITH_TAG_(CAdapterCommon, Unknown, UnknownOuter, PoolType, 'rCcP', PADAPTERCOMMON);

    CAdapterCommon *p = new(PoolType, 'rCcP') CAdapterCommon(UnknownOuter);
    if (p)
    {
        *Unknown = PUNKNOWN((PADAPTERCOMMON)(p));
        (*Unknown)->AddRef();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    return ntStatus;
} 

CAdapterCommon::CAdapterCommon(PUNKNOWN pUnknownOuter):CUnknown( pUnknownOuter )
{
	PAGED_CODE();

	//DPF_ENTER(("[CAdapterCommon::CAdapterCommon]"));
	m_PowerState  = PowerDeviceD0;
}

CAdapterCommon::~CAdapterCommon(void)
{
	PAGED_CODE();
    //DPF_ENTER(("[CAdapterCommon::~CAdapterCommon]"));

    if (m_pHW)
        delete m_pHW;

    if (m_pPortWave)
	    m_pPortWave->Release();

	if (m_pServiceGroupWave)
	    m_pServiceGroupWave->Release();
}

// Returns the deviceobject
STDMETHODIMP_(PDEVICE_OBJECT) CAdapterCommon::GetDeviceObject(void)
{
    PAGED_CODE();
    
    return m_pDeviceObject;
}

/* Initialize adapter common object.
Arguments: DeviceObject - pointer to the device object */
NTSTATUS CAdapterCommon::Init (IN  PDEVICE_OBJECT DeviceObject)
{
	PAGED_CODE();
	ASSERT(DeviceObject);

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    //DPF_ENTER(("[CAdapterCommon::Init]"));

	m_pDeviceObject = DeviceObject;
	m_PowerState    = PowerDeviceD0;

    // Initialize HW.
    m_pHW = new (NonPagedPool, SONICSAUDIO_POOLTAG)  AudioHW;
    if (!m_pHW)
    {
        //DPF(D_TERSE, ("Insufficient memory for MSVAD HW"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        m_pHW->MixerReset();
    }

    return ntStatus;
}

//Reset mixer registers from registry.
STDMETHODIMP_(void) CAdapterCommon::MixerReset(void)
{
	PAGED_CODE();
    
    if (m_pHW)
    {
        m_pHW->MixerReset();
    }
}

// QueryInterface routine for AdapterCommon
STDMETHODIMP CAdapterCommon::NonDelegatingQueryInterface(REFIID Interface, PVOID *Object)
{
	PAGED_CODE();
	ASSERT(Object);

	if (IsEqualGUIDAligned(Interface, IID_IUnknown))
	{
		*Object = PVOID(PUNKNOWN(PADAPTERCOMMON(this)));
	}
	else if (IsEqualGUIDAligned(Interface, IID_IAdapterCommon))
	{
		*Object = PVOID(PADAPTERCOMMON(this));
	}
	else if (IsEqualGUIDAligned(Interface, IID_IAdapterPowerManagement))
	{
		*Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
	}
	else
	{
		*Object = NULL;
	}

	if (*Object)
	{
		PUNKNOWN(*Object)->AddRef();
		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_PARAMETER;
}

STDMETHODIMP_(void) CAdapterCommon::SetWaveServiceGroup(IN PSERVICEGROUP ServiceGroup)
{
    PAGED_CODE();
    //DPF_ENTER(("[CAdapterCommon::SetWaveServiceGroup]"));
    
    if (m_pServiceGroupWave)
	{
		m_pServiceGroupWave->Release();
	}

	m_pServiceGroupWave = ServiceGroup;

	if (m_pServiceGroupWave)
	{
		m_pServiceGroupWave->AddRef();
	}
}

// Returns the waveport pointer.
STDMETHODIMP_(PUNKNOWN *) CAdapterCommon::WavePortDriverDest(void)
{
	PAGED_CODE();

	return (PUNKNOWN *)&m_pPortWave;
}
#pragma code_seg()

// Fetch Device Specific information.
STDMETHODIMP_(BOOL) CAdapterCommon::bDevSpecificRead()
{
    if (m_pHW)
    {
        return m_pHW->bGetDevSpecific();
    }

    return FALSE;
}

/* Store the new value in the Device Specific location.
   Arguments:  bDevSpecific - Value to store */
STDMETHODIMP_(void) CAdapterCommon::bDevSpecificWrite(IN BOOL bDevSpecific)
{
    if (m_pHW)
    {
        m_pHW->bSetDevSpecific(bDevSpecific);
    }
}

// Fetch Device Specific INT information.
STDMETHODIMP_(INT) CAdapterCommon::iDevSpecificRead()
{
    if (m_pHW)
    {
        return m_pHW->iGetDevSpecific();
    }

    return 0;
}

/* Store the new value in the Device Specific location.
   Arguments: iDevSpecific - Value to store */
STDMETHODIMP_(void) CAdapterCommon::iDevSpecificWrite(IN INT iDevSpecific)
{
    if (m_pHW)
    {
        m_pHW->iSetDevSpecific(iDevSpecific);
    }
}

// Fetch Device Specific UINT information.
STDMETHODIMP_(UINT) CAdapterCommon::uiDevSpecificRead()
{
    if (m_pHW)
    {
        return m_pHW->uiGetDevSpecific();
    }

    return 0;
}

/* Store the new value in the Device Specific location.
   Arguments:  uiDevSpecific - Value to store */
STDMETHODIMP_(void) CAdapterCommon::uiDevSpecificWrite(IN UINT uiDevSpecific)
{
    if (m_pHW)
    {
        m_pHW->uiSetDevSpecific(uiDevSpecific);
    }
}

/* Store the new value in mixer register array.
   Arguments: Index - node id
   Return Value: BOOL - mixer mute setting for this node */
STDMETHODIMP_(BOOL) CAdapterCommon::MixerMuteRead(IN ULONG Index)
{
    if (m_pHW)
    {
        return m_pHW->GetMixerMute(Index);
    }

    return 0;
}

/*  Store the new value in mixer register array.
    Arguments:  Index - node id
                Value - new mute settings */
STDMETHODIMP_(void) CAdapterCommon::MixerMuteWrite(IN ULONG Index, IN  BOOL Value)
{
    if (m_pHW)
    {
        m_pHW->SetMixerMute(Index, Value);
    }
}

// Return the mux selection
STDMETHODIMP_(ULONG) CAdapterCommon::MixerMuxRead() 
{
    if (m_pHW)
    {
        return m_pHW->GetMixerMux();
    }

    return 0;
}

STDMETHODIMP_(void)
CAdapterCommon::MixerMuxWrite
(
    IN  ULONG                   Index
)
/*++  Store the new mux selection
      Arguments: Index - node id
                 Value - new mute settings */
{
    if (m_pHW)
    {
        m_pHW->SetMixerMux(Index);
    }
}

/* Arguments:  Index - node id
   Channel = which channel
   Return Value: mixer volume settings for this line */
STDMETHODIMP_(LONG) CAdapterCommon::MixerVolumeRead(IN ULONG Index, IN  LONG Channel)
{
    if (m_pHW)
    {
        return m_pHW->GetMixerVolume(Index, Channel);
    }

    return 0;
}

/* Store the new value in mixer register array.
Arguments:  Index - node id
            Channel - which channel
            Value - new volume level */
STDMETHODIMP_(void) CAdapterCommon::MixerVolumeWrite( 	IN  ULONG Index, IN  LONG Channel, IN  LONG Value)
{
    if (m_pHW)
    {
        m_pHW->SetMixerVolume(Index, Channel, Value);
    }
}

// Arguments: NewState - The requested, new power state for the device. 
STDMETHODIMP_(void) CAdapterCommon::PowerChangeState(IN  POWER_STATE NewState)
{
    //UINT i;
    //DPF_ENTER(("[CAdapterCommon::PowerChangeState]"));

    // is this actually a state change??
    if (NewState.DeviceState != m_PowerState)
    {
        // switch on new state
        switch (NewState.DeviceState)
        {
            case PowerDeviceD0:
            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:
                m_PowerState = NewState.DeviceState;

//              DPF(D_VERBOSE, ("Entering D%d", ULONG(m_PowerState) - ULONG(PowerDeviceD0)));

                break;
    
            default:
            
                //DPF(D_VERBOSE, ("Unknown Device Power State"));
                break;
        }
    }
} // PowerStateChange

/*  Called at startup to get the caps for the device.  This structure provides
    the system with the mappings between system power state and device power
    state.  This typically will not need modification by the driver.
    Arguments: PowerDeviceCaps - The device's capabilities. */
STDMETHODIMP_(NTSTATUS) CAdapterCommon::QueryDeviceCapabilities ( IN  PDEVICE_CAPABILITIES    PowerDeviceCaps)
{
	UNREFERENCED_PARAMETER(PowerDeviceCaps);
    //DPF_ENTER(("[CAdapterCommon::QueryDeviceCapabilities]"));

    return (STATUS_SUCCESS);
} 

/*  Query to see if the device can change to this power state
    Arguments:  NewStateQuery - The requested, new power state for the device */
STDMETHODIMP_(NTSTATUS) CAdapterCommon::QueryPowerChangeState (IN  POWER_STATE NewStateQuery)
{
	UNREFERENCED_PARAMETER(NewStateQuery);
    //DPF_ENTER(("[CAdapterCommon::QueryPowerChangeState]"));

    return STATUS_SUCCESS;
}