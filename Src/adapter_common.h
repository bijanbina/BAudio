#ifndef __ADAPTER_COMMON_H_
#define __ADAPTER_COMMON_H_

class CAdapterCommon : public IAdapterCommon, public IAdapterPowerManagement, public CUnknown
{
public:
    DECLARE_STD_UNKNOWN();
    CAdapterCommon(PUNKNOWN pUnknownOuter);
    ~CAdapterCommon();

    IMP_IAdapterPowerManagement;

    STDMETHODIMP_(NTSTATUS) Init(IN PDEVICE_OBJECT DeviceObject);
    STDMETHODIMP_(PDEVICE_OBJECT) GetDeviceObject(void);
    STDMETHODIMP_(PUNKNOWN*) WavePortDriverDest(void);
    STDMETHODIMP_(void) SetWaveServiceGroup(IN PSERVICEGROUP ServiceGroup);

    STDMETHODIMP_(BOOL) bDevSpecificRead();

    STDMETHODIMP_(void) bDevSpecificWrite(IN BOOL bDevSpecific);
    STDMETHODIMP_(INT) iDevSpecificRead();

    STDMETHODIMP_(void) iDevSpecificWrite(IN INT iDevSpecific);
    STDMETHODIMP_(UINT) uiDevSpecificRead();

    STDMETHODIMP_(void) uiDevSpecificWrite(IN  UINT uiDevSpecific);

    STDMETHODIMP_(BOOL) MixerMuteRead(IN ULONG Index);

    STDMETHODIMP_(void) MixerMuteWrite(IN ULONG Index, IN BOOL Value);

    STDMETHODIMP_(ULONG) MixerMuxRead(void);

    STDMETHODIMP_(void) MixerMuxWrite(IN ULONG Index);

    STDMETHODIMP_(void) MixerReset(void);

    STDMETHODIMP_(LONG) MixerVolumeRead(IN ULONG Index, IN LONG Channel);

    STDMETHODIMP_(void) MixerVolumeWrite(IN ULONG Index, IN LONG Channel, IN LONG Value);

    friend NTSTATUS NewAdapterCommon(OUT PADAPTERCOMMON* OutAdapterCommon,
        IN  PRESOURCELIST   ResourceList);

private:
    PPORTWAVECYCLIC         m_pPortWave;    // Port interface
    PSERVICEGROUP           m_pServiceGroupWave;
    PDEVICE_OBJECT          m_pDeviceObject;
    DEVICE_POWER_STATE      m_PowerState;
    PAudioHW                m_pHW;          // VAD HW
};

#endif  //__ADAPTER_COMMON_H_

