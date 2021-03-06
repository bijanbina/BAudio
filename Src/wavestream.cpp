//  WaveCyclicStream-Miniport and IDmaChannel implementation. Does nothing HW related.
#include "sonicsaudio.h"
#include "common.h"
#include "wave.h"
#include "wavestream.h"

#define DBGMESSAGE "[SONICS-Audio] wavestream.cpp: "
#define DBGPRINT(x) DbgPrint(DBGMESSAGE x)

#pragma code_seg("PAGE")
CMiniportWaveCyclicStream::CMiniportWaveCyclicStream(PUNKNOWN pUnknownOuter):CUnknown( pUnknownOuter )
{
    PAGED_CODE();
    //DPF_ENTER(("[CMiniportWaveCyclicStream::CMiniportWaveCyclicStream]"));
	m_pMiniport =  NULL;
	m_fCapture = FALSE; 
	m_fFormat8Bit = FALSE;
	m_usBlockAlign = 0;
	m_ulPin = (ULONG)-1;

	m_fDmaActive = FALSE;
    m_ulDmaPosition = 0;
    m_ullElapsedTimeCarryForward = 0;
    m_ulByteDisplacementCarryForward = 0;
	m_pvDmaBuffer = NULL;
    m_ulDmaBufferSize = 0;
    m_ulDmaMovementRate = 0;
    m_ullDmaTimeStamp = 0;

    m_ksState = KSSTATE_STOP ; 

	m_pDpc = NULL; 
	m_pTimer = NULL; 	
}

// Destructor for wavecyclicstream 
CMiniportWaveCyclicStream::~CMiniportWaveCyclicStream(void)
{
  PAGED_CODE();
  //DPF_ENTER(("[CMiniportWaveCyclicStream::~CMiniportWaveCyclicStream]"));

  if (NULL != m_pMiniport)
  {
      if (m_fCapture)
          m_pMiniport->m_fCaptureAllocated = FALSE;
      else
          m_pMiniport->m_fRenderAllocated = FALSE;
  }
  if (m_pTimer)
  {
      KeCancelTimer(m_pTimer);
      ExFreePool(m_pTimer);
  }

  if (m_pDpc)
      ExFreePool( m_pDpc );

  // Free the DMA buffer
  FreeBuffer();

}

//  Initializes the stream object. Allocate a DMA buffer, timer and DPC
NTSTATUS CMiniportWaveCyclicStream::Init(IN PCMiniportWaveCyclic Miniport_, 
    IN ULONG Pin_, IN BOOLEAN Capture_,  IN PKSDATAFORMAT DataFormat_)
{
  PAGED_CODE();
  //DPF_ENTER(("[CMiniportWaveCyclicStream::Init]"));
  ASSERT(Miniport_);
  ASSERT(DataFormat_);

  m_pMiniport = Miniport_;

  m_fCapture = FALSE;
  m_fFormat8Bit = FALSE;
  m_usBlockAlign = 0;
  m_ksState = KSSTATE_STOP;
  m_ulPin = (ULONG)-1;

  m_pDpc = NULL;
  m_pTimer = NULL;

  m_fDmaActive = FALSE;
  m_ulDmaPosition = 0;

  m_ullElapsedTimeCarryForward = 0;
  m_ulByteDisplacementCarryForward = 0;

  m_pvDmaBuffer = NULL;
  m_ulDmaBufferSize = 0;
  m_ulDmaMovementRate = 0;    
  m_ullDmaTimeStamp = 0;

  NTSTATUS ntStatus = STATUS_SUCCESS;
  PWAVEFORMATEX pWfx;

  pWfx = GetWaveFormatEx(DataFormat_);
  if (!pWfx)
  {
    //DPF(D_TERSE, ("Invalid DataFormat param in NewStream"));
    ntStatus = STATUS_INVALID_PARAMETER;
  }

  if (NT_SUCCESS(ntStatus))
  {
    m_ulPin         = Pin_;
    m_fCapture      = Capture_;
    m_usBlockAlign  = pWfx->nBlockAlign;
    m_fFormat8Bit  = (pWfx->wBitsPerSample == 8);
    m_ksState       = KSSTATE_STOP;
    m_ulDmaPosition = 0;

    m_ullElapsedTimeCarryForward = 0;
    m_ulByteDisplacementCarryForward = 0;

    m_fDmaActive    = FALSE;
    m_pDpc          = NULL;
    m_pTimer        = NULL;
    m_pvDmaBuffer   = NULL;
  }

  // Allocate DMA buffer for this stream.
  if (NT_SUCCESS(ntStatus))
  {
      ntStatus = AllocateBuffer(m_pMiniport->m_MaxDmaBufferSize, NULL);
  }

  // Set sample frequency. Note that m_SampleRateSync access should
  // be syncronized.
  if (NT_SUCCESS(ntStatus))
  {
    ntStatus = KeWaitForSingleObject( &m_pMiniport->m_SampleRateSync,
      Executive, KernelMode, FALSE, NULL);

    if (NT_SUCCESS(ntStatus))
    {
      m_pMiniport->m_SamplingFrequency = pWfx->nSamplesPerSec;
      KeReleaseMutex(&m_pMiniport->m_SampleRateSync, FALSE);
    }
    else
    {
      //DPF(D_TERSE, ("[SamplingFrequency Sync failed: %08X]", ntStatus));
    }
  }

  if (NT_SUCCESS(ntStatus))
  {
      ntStatus = SetFormat(DataFormat_);
  }

  if (NT_SUCCESS(ntStatus))
  {
      m_pDpc = (PRKDPC) ExAllocatePoolWithTag(
        NonPagedPool, sizeof(KDPC), SONICSAUDIO_POOLTAG);
      if (!m_pDpc)
      {
        //DPF(D_TERSE, ("[Could not allocate memory for DPC]"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
      }
  }

  if (NT_SUCCESS(ntStatus))
  {
    m_pTimer = (PKTIMER) ExAllocatePoolWithTag(
      NonPagedPool, sizeof(KTIMER), SONICSAUDIO_POOLTAG);
    if (!m_pTimer) 
    {
      //DPF(D_TERSE, ("[Could not allocate memory for Timer]"));
      ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
  }

  if (NT_SUCCESS(ntStatus))
  {
    KeInitializeDpc(m_pDpc, TimerNotify, m_pMiniport);
    KeInitializeTimerEx(m_pTimer, NotificationTimer);
  }

  return ntStatus;
} // Init

/*  QueryInterface
    Arguments:  Interface - GUID
    Object - interface pointer to be returned */
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclicStream::NonDelegatingQueryInterface( 
  IN REFIID Interface, OUT PVOID * Object)
{
  PAGED_CODE();
  ASSERT(Object);

  if (IsEqualGUIDAligned(Interface, IID_IUnknown)) {
    *Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLICSTREAM(this)));
  } else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclicStream)) {
    *Object = PVOID(PMINIPORTWAVECYCLICSTREAM(this));
  } else if (IsEqualGUIDAligned(Interface, IID_IDmaChannel)) {
    *Object = PVOID(PDMACHANNEL(this));
  } else {
    *Object = NULL;
  }

  if (*Object) {
    PUNKNOWN(*Object)->AddRef();
    return STATUS_SUCCESS;
  }

  return STATUS_INVALID_PARAMETER;
} // NonDelegatingQueryInterface
#pragma code_seg()

/*  The GetPosition function gets the current position of the DMA read or write
    pointer for the stream. Callers of GetPosition should run at IRQL <= DISPATCH_LEVEL.
    Arguments:  Position - Position of the DMA pointer */
STDMETHODIMP CMiniportWaveCyclicStream::GetPosition(OUT PULONG Position)
{
  if (m_fDmaActive) {
    ULONGLONG CurrentTime = KeQueryInterruptTime();

    // Calculate the time elapsed since the last call to GetPosition() or since the
    // DMA engine started.  Note that the division by 10000 to convert to milliseconds
    // may cause us to lose some of the time, so we will carry the remainder forward 
    // to the next GetPosition() call.

    //ULONG TimeElapsedInMS = ( (ULONG) (CurrentTime - m_ullDmaTimeStamp) ) / 10000;
	ULONG TimeElapsedInMS = ( (ULONG) (CurrentTime - m_ullDmaTimeStamp + m_ullElapsedTimeCarryForward) ) / 10000;
    // Carry forward the remainder of this division so we don't fall behind with our position.
    //
    m_ullElapsedTimeCarryForward = (CurrentTime - m_ullDmaTimeStamp + m_ullElapsedTimeCarryForward) % 10000;
    // Calculate how many bytes in the DMA buffer would have been processed in the elapsed
    // time.  Note that the division by 1000 to convert to milliseconds may cause us to 
    // lose some bytes, so we will carry the remainder forward to the next GetPosition() call.
    //
    //ULONG ByteDisplacement = (m_ulDmaMovementRate * TimeElapsedInMS) / 1000;
    ULONG ByteDisplacement = ((m_ulDmaMovementRate * TimeElapsedInMS) + m_ulByteDisplacementCarryForward) / 1000;
    // Increment the DMA position by the number of bytes displaced since the last
    // call to GetPosition() and ensure we properly wrap at buffer length.
    //
    m_ulDmaPosition = (m_ulDmaPosition + ByteDisplacement) % m_ulDmaBufferSize;
    // Return the new DMA position
    //
    *Position = m_ulDmaPosition;
    // Update the DMA time stamp for the next call to GetPosition()
    //
    m_ullDmaTimeStamp = CurrentTime;
  } else {
    // DMA is inactive so just return the current DMA position.
    //
    *Position = m_ulDmaPosition;
  }

  return STATUS_SUCCESS;
} // GetPosition

/*  Given a physical position based on the actual number of bytes transferred,
    NormalizePhysicalPosition converts the position to a time-based value of
    100 nanosecond units. Callers of NormalizePhysicalPosition can run at any IRQL.

Arguments: PhysicalPosition - On entry this variable contains the value to convert.
                              On return it contains the converted value */
STDMETHODIMP CMiniportWaveCyclicStream::NormalizePhysicalPosition(IN OUT PLONGLONG PhysicalPosition)
{
  *PhysicalPosition = ( _100NS_UNITS_PER_SECOND / m_usBlockAlign * *PhysicalPosition ) / m_pMiniport->m_SamplingFrequency;
  return STATUS_SUCCESS;
} // NormalizePhysicalPosition

#pragma code_seg("PAGE")
/*  The SetFormat function changes the format associated with a stream.
  Callers of SetFormat should run at IRQL PASSIVE_LEVEL
Arguments:  Format - Pointer to a KSDATAFORMAT structure which indicates the new format
                     of the stream. */
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclicStream::SetFormat(IN  PKSDATAFORMAT Format)
{
  PAGED_CODE();
  ASSERT(Format);
  //DPF_ENTER(("[CMiniportWaveCyclicStream::SetFormat]"));

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
  PWAVEFORMATEX pWfx;

  if (m_ksState != KSSTATE_RUN) {
    //First validate the format
    NTSTATUS ntValidFormat;
    ntValidFormat = m_pMiniport->ValidateFormat(Format);
    if (NT_SUCCESS(ntValidFormat)) {
      pWfx = GetWaveFormatEx(Format);
      if (pWfx) {
        ntStatus = KeWaitForSingleObject(
          &m_pMiniport->m_SampleRateSync,
          Executive,
          KernelMode,
          FALSE,
          NULL
        );
        if (NT_SUCCESS(ntStatus)) {
            m_usBlockAlign  = pWfx->nBlockAlign;
            m_fFormat8Bit  = (pWfx->wBitsPerSample == 8);
            m_pMiniport->m_SamplingFrequency = pWfx->nSamplesPerSec;
            m_ulDmaMovementRate = pWfx->nAvgBytesPerSec;

            //DPF(D_TERSE, ("New Format: %d", pWfx->nSamplesPerSec));
        }
        KeReleaseMutex(&m_pMiniport->m_SampleRateSync, FALSE);
      }
    }
  }

  return ntStatus;
}

/*  The SetNotificationFrequency function sets the frequency at which
    notification interrupts are generated. Callers of SetNotificationFrequency
    should run at IRQL PASSIVE_LEVEL.

Arguments: Interval - Value indicating the interval between interrupts,
                      expressed in milliseconds
           FramingSize - Pointer to a ULONG value where the number of bytes equivalent
                         to Interval milliseconds is returned */
STDMETHODIMP_(ULONG) CMiniportWaveCyclicStream::SetNotificationFreq(IN ULONG Interval, OUT PULONG FramingSize)
{
  PAGED_CODE();
  ASSERT(FramingSize);
  //DPF_ENTER(("[CMiniportWaveCyclicStream::SetNotificationFreq]"));

  m_pMiniport->m_NotificationInterval = Interval;

  *FramingSize = m_usBlockAlign * m_pMiniport->m_SamplingFrequency * Interval / 1000;

  return m_pMiniport->m_NotificationInterval;
}

/*  The SetState function sets the new state of playback or recording for the
    stream. SetState should run at IRQL PASSIVE_LEVEL
    Arguments: NewState - KSSTATE indicating the new state for the stream. */
STDMETHODIMP CMiniportWaveCyclicStream::SetState(IN  KSSTATE NewState)
{
  PAGED_CODE();
  //DPF_ENTER(("[CMiniportWaveCyclicStream::SetState]"));

  NTSTATUS ntStatus = STATUS_SUCCESS;

  // The acquire state is not distinguishable from the stop state for our purposes.
  if (NewState == KSSTATE_ACQUIRE) {
    NewState = KSSTATE_STOP;
  }

  if (m_ksState != NewState) {
    switch(NewState) {
      case KSSTATE_PAUSE:
        //DPF(D_TERSE, ("KSSTATE_PAUSE"));
        m_fDmaActive = FALSE;
        break;

      case KSSTATE_RUN:
        //DPF(D_TERSE, ("KSSTATE_RUN"));

		LARGE_INTEGER   delay;

        // Set the timer for DPC.
        m_ullDmaTimeStamp   = KeQueryInterruptTime();
		////
		m_ullElapsedTimeCarryForward  = 0;
        m_fDmaActive        = TRUE;
        delay.HighPart      = 0;
        delay.LowPart       = m_pMiniport->m_NotificationInterval;

        KeSetTimerEx(m_pTimer, delay, m_pMiniport->m_NotificationInterval, m_pDpc);
        break;

      case KSSTATE_STOP:
        //DPF(D_TERSE, ("KSSTATE_STOP"));

        m_fDmaActive = FALSE;
        m_ulDmaPosition = 0;
		////
        m_ullElapsedTimeCarryForward      = 0;
        m_ulByteDisplacementCarryForward  = 0;

        KeCancelTimer( m_pTimer );
        break;
    }

    m_ksState = NewState;
  }

  return ntStatus;
}

#pragma code_seg()

/* The Silence function is used to copy silence samplings to a certain location.
   Callers of Silence can run at any IRQL
   Arguments: Buffer - Pointer to the buffer where the silence samplings should
              be deposited.
              ByteCount - Size of buffer indicating number of bytes to be deposited. */
STDMETHODIMP_(void) CMiniportWaveCyclicStream::Silence(IN PVOID Buffer, IN ULONG ByteCount)
{
  RtlFillMemory(Buffer, ByteCount, m_fFormat8Bit ? 0x80 : 0);
}

#pragma code_seg("PAGE")

STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclicStream::AllocateBuffer( 
  IN ULONG BufferSize, IN PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL)
{
  
  UNREFERENCED_PARAMETER(PhysicalAddressConstraint); 

  PAGED_CODE();

  DBGPRINT("[CMiniportWaveCyclicStream::AllocateBuffer]");

  // Adjust this cap as needed...
  ASSERT (BufferSize <= DMA_BUFFER_SIZE);

  //NTSTATUS ntStatus = STATUS_SUCCESS;

  //m_pvDmaBuffer = (PVOID) ExAllocatePoolWithTag(NonPagedPool, BufferSize, SONICSAUDIO_POOLTAG);
  //if (!m_pvDmaBuffer) {
  //    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
  //} else {
     m_ulDmaBufferSize = BufferSize;
  //}

  return STATUS_SUCCESS;
}
#pragma code_seg()

/*  AllocatedBufferSize returns the size of the allocated buffer.
   Callers of AllocatedBufferSize can run at any IRQL.*/
STDMETHODIMP_(ULONG) CMiniportWaveCyclicStream::AllocatedBufferSize(void)
{
  DBGPRINT("[CMiniportWaveCyclicStream::AllocatedBufferSize]");
  return DMA_BUFFER_SIZE;
} // AllocatedBufferSize

/*  BufferSize returns the size set by SetBufferSize or the allocated buffer size
    if the buffer size has not been set. The DMA object does not actually use
    this value internally. This value is maintained by the object to allow its
    various clients to communicate the intended size of the buffer. This call
    is often used to obtain the map size parameter to the Start member
    function. Callers of BufferSize can run at any IRQL */
STDMETHODIMP_(ULONG) CMiniportWaveCyclicStream::BufferSize(void)
{
  return DMA_BUFFER_SIZE;
}

/*  The CopyFrom function copies sample data from the DMA buffer.
    Callers of CopyFrom can run at any IRQL */
STDMETHODIMP_(void) CMiniportWaveCyclicStream::CopyFrom( 
    IN PVOID Destination, IN PVOID Source, IN  ULONG ByteCount )
{
  UNREFERENCED_PARAMETER(Source);

	KeAcquireSpinLock( &m_pMiniport->spin_lock , &m_pMiniport->spin_irql );
	PUCHAR pucTranBuf = (PUCHAR)m_pMiniport->m_pvTranBuf;

	if( m_pMiniport->m_lDataLen <= LONG(ByteCount )){
		if( m_pMiniport->m_lDataLen < LONG(ByteCount ) ) 
			RtlFillMemory( (PUCHAR)Destination + m_pMiniport->m_lDataLen, ByteCount - m_pMiniport->m_lDataLen , m_fFormat8Bit ? 0x80 : 0 ); 
		if( m_pMiniport->m_lDataLen > 0){
			RtlCopyMemory( Destination, pucTranBuf + m_pMiniport->m_lRecPos, m_pMiniport->m_lDataLen );
		}
		m_pMiniport->m_lRecPos = 0 ;
		m_pMiniport->m_lDataLen = 0; 
	}
	else{
		RtlCopyMemory( Destination, pucTranBuf + m_pMiniport->m_lRecPos, ByteCount );
		m_pMiniport->m_lRecPos  += ByteCount ; 
		m_pMiniport->m_lDataLen -= ByteCount;
	}
	/////
//	DPT("Handle=%p, Rec_Buf=%p, Rec_len=%d\n", hdt,buf,len);

	KeReleaseSpinLock( &m_pMiniport->spin_lock,  m_pMiniport->spin_irql );
}

/*  The CopyTo function copies sample data to the DMA buffer.
    Callers of CopyTo can run at any IRQL.*/
STDMETHODIMP_(void) CMiniportWaveCyclicStream::CopyTo(IN  PVOID Destination, IN PVOID Source, IN ULONG ByteCount)
{
  UNREFERENCED_PARAMETER(Destination);
 /////
	if( ByteCount > m_pMiniport->m_ulTranBufSize ) {
		DBGPRINT("Shit Too Large Play Buffer.\n");
		return;
	}
	//////
	KeAcquireSpinLock( &m_pMiniport->spin_lock , &m_pMiniport->spin_irql );
	PUCHAR pucTranBuf = (PUCHAR)m_pMiniport->m_pvTranBuf;

	if( m_pMiniport->m_lRecPos + m_pMiniport->m_lDataLen + ByteCount > m_pMiniport->m_ulTranBufSize ){ 
		if( m_pMiniport->m_lDataLen + ByteCount > m_pMiniport->m_ulTranBufSize ){ 
			m_pMiniport->m_lRecPos = 0; 
			RtlCopyMemory( pucTranBuf, Source, ByteCount ); 
			m_pMiniport->m_lDataLen = ByteCount; 
		}
		else{
			RtlMoveMemory( pucTranBuf, pucTranBuf + m_pMiniport->m_lRecPos, m_pMiniport->m_lDataLen ); 
			m_pMiniport->m_lRecPos = 0; 
			RtlCopyMemory( pucTranBuf + m_pMiniport->m_lDataLen, Source, ByteCount );
			m_pMiniport->m_lDataLen += ByteCount ; 
		}
	}
	else{
		RtlCopyMemory( pucTranBuf + m_pMiniport->m_lRecPos + m_pMiniport->m_lDataLen, Source, ByteCount );
		m_pMiniport->m_lDataLen += ByteCount ; 
	}
//	DPT("Handle=%p, Play_Buf=%p, Play_len=%d\n", hdt, buf, len ); 

	KeReleaseSpinLock( &m_pMiniport->spin_lock,  m_pMiniport->spin_irql );
} // CopyTo

/*  The FreeBuffer function frees the buffer allocated by AllocateBuffer. Because
    the buffer is automatically freed when the DMA object is deleted, this
    function is not normally used. Callers of FreeBuffer should run at IRQL PASSIVE_LEVEL. */#pragma code_seg("PAGE")
STDMETHODIMP_(void) CMiniportWaveCyclicStream::FreeBuffer(void)
{
  DBGPRINT("[CMiniportWaveCyclicStream::FreeBuffer]");

  //if ( m_pvDmaBuffer ) {
   // ExFreePool( m_pvDmaBuffer );
    m_ulDmaBufferSize = 0;
   // m_pvDmaBuffer = NULL;
  //}

} // FreeBuffer
#pragma code_seg()

/*  The GetAdapterObject function returns the DMA object's internal adapter
    object. Callers of GetAdapterObject can run at any IRQL. */
STDMETHODIMP_(PADAPTER_OBJECT) CMiniportWaveCyclicStream::GetAdapterObject(void)
{
  DBGPRINT("[CMiniportWaveCyclicStream::GetAdapterObject]");

  // MSVAD does not have need a physical DMA channel. Therefore it 
  // does not have physical DMA structure.
    
  return NULL;
} // GetAdapterObject

STDMETHODIMP_(ULONG) CMiniportWaveCyclicStream::MaximumBufferSize(void)
{
  DBGPRINT("[CMiniportWaveCyclicStream::MaximumBufferSize]");
  return DMA_BUFFER_SIZE;;
} // MaximumBufferSize

/* MaximumBufferSize returns the size in bytes of the largest buffer this DMA
   object is configured to support. Callers of MaximumBufferSize can run
   at any IRQL */
STDMETHODIMP_(PHYSICAL_ADDRESS) CMiniportWaveCyclicStream::PhysicalAddress(void)
{
  DBGPRINT("[CMiniportWaveCyclicStream::PhysicalAddress]");

  PHYSICAL_ADDRESS pAddress;
  pAddress.QuadPart = (LONGLONG) m_pMiniport->m_pvTranBuf;
  return pAddress;
}

/* The SetBufferSize function sets the current buffer size. This value is set to
   the allocated buffer size when AllocateBuffer is called. The DMA object does
   not actually use this value internally. This value is maintained by the object
   to allow its various clients to communicate the intended size of the buffer.
   Callers of SetBufferSize can run at any IRQL.
   Arguments:  BufferSize - Current size in bytes. */
STDMETHODIMP_(void) CMiniportWaveCyclicStream::SetBufferSize(IN ULONG BufferSize)

{
  DBGPRINT("[CMiniportWaveCyclicStream::SetBufferSize]");

  if ( BufferSize <= m_ulDmaBufferSize ) 
  {
    m_ulDmaBufferSize = BufferSize;
  }
  else 
  {
    //DPF(D_ERROR, ("Tried to enlarge dma buffer size"));
  }
}

/*  The SystemAddress function returns the virtual system address of the
    allocated buffer. Callers of SystemAddress can run at any IRQL.
    Return Value:   PVOID - The return value is the virtual system address of the
                            allocated buffer. */
STDMETHODIMP_(PVOID) CMiniportWaveCyclicStream::SystemAddress(void)
{
  return m_pMiniport->m_pvTranBuf;
}

/* The TransferCount function returns the size in bytes of the buffer currently
   being transferred by a slave DMA object. Callers of TransferCount can run
   at any IRQL.
   Return Value:  ULONG - The return value is the size in bytes of the buffer currently
                          being transferred. */
STDMETHODIMP_(ULONG) CMiniportWaveCyclicStream::TransferCount(void)
{
  DBGPRINT("[CMiniportWaveCyclicStream::TransferCount]");
  return DMA_BUFFER_SIZE;
}
