// Definition of wavecyclic miniport class.

#ifndef __WAVESTREAM_H_
#define __WAVESTREAM_H_

#include "wave.h"

// CMiniportWaveCyclicStream 

class CMiniportWaveCyclicStream : public IMiniportWaveCyclicStream, public IDmaChannel, public CUnknown {
protected:
  PCMiniportWaveCyclic      m_pMiniport;        // Miniport that created us  
  BOOLEAN                   m_fCapture;         // Capture or render.
  BOOLEAN                   m_fFormat8Bit;      // Is 8-bit samples?
  USHORT                    m_usBlockAlign;     // Block alignment of current format.
  KSSTATE                   m_ksState;          // Stop, pause, run.
  ULONG                     m_ulPin;            // Pin Id.

  PRKDPC                    m_pDpc;             // Deferred procedure call object
  PKTIMER                   m_pTimer;           // Timer object

  BOOLEAN                   m_fDmaActive;       // Dma currently active? 
  ULONG                     m_ulDmaPosition;    // Position in Dma
  PVOID                     m_pvDmaBuffer;      // Dma buffer pointer
  ULONG                     m_ulDmaBufferSize;  // Size of dma buffer
  ULONG                     m_ulDmaMovementRate;// Rate of transfer specific to system
  ULONGLONG                 m_ullDmaTimeStamp;  // Dma time elasped 
  ////
  ULONGLONG                 m_ullElapsedTimeCarryForward;       // Time to carry forward in position calc.
  ULONG                     m_ulByteDisplacementCarryForward;   // Bytes to carry forward to next calc.
  
public:
    DECLARE_STD_UNKNOWN();
    //DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicStream);
	CMiniportWaveCyclicStream(PUNKNOWN pUnknownOuter);
    ~CMiniportWaveCyclicStream();

	IMP_IMiniportWaveCyclicStream;
    IMP_IDmaChannel;

    NTSTATUS Init(IN  PCMiniportWaveCyclic Miniport,
        IN ULONG Channel,IN BOOLEAN Capture, IN PKSDATAFORMAT DataFormat);

    // Friends
    friend class CMiniportWaveCyclic;
    //STDMETHOD_(NTSTATUS __stdcall, AllocateBuffer)(IN ULONG BufferSize, IN PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL);
    //STDMETHOD_(ULONG __stdcall, AllocatedBufferSize)(void);
    //STDMETHOD_(ULONG __stdcall, BufferSize)(void);
    //STDMETHOD_(void __stdcall, CopyFrom)(IN PVOID Destination, IN PVOID Source, IN ULONG ByteCount);
    //STDMETHOD_(void __stdcall, CopyTo)(IN PVOID Destination, IN PVOID Source, IN ULONG ByteCount);
    //STDMETHOD_(void __stdcall, FreeBuffer)(void);
    //STDMETHOD_(PADAPTER_OBJECT __stdcall, GetAdapterObject)(void);
    //STDMETHOD_(ULONG __stdcall, MaximumBufferSize)(void);
    //STDMETHOD_(PHYSICAL_ADDRESS __stdcall, PhysicalAddress)(void);
    //STDMETHOD_(void __stdcall, SetBufferSize)(IN ULONG BufferSize);
    //STDMETHOD_(PVOID __stdcall, SystemAddress)(void);
    //STDMETHOD_(ULONG __stdcall, TransferCount)(void);
};
typedef CMiniportWaveCyclicStream *PCMiniportWaveCyclicStream;

#endif

