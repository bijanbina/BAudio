//Virtual Hardware (settings and register)

/*  MSVAD HW has an array for storing mixer and volume settings
    for the topology. */
#include "sonicsaudio.h"
#include "hw.h"

#pragma code_seg("PAGE")
AudioHW::AudioHW(): m_ulMux(0), m_bDevSpecific(FALSE),
    m_iDevSpecific(0), m_uiDevSpecific(0)
{
    PAGED_CODE();
    
    MixerReset();
}
#pragma code_seg()

//  Gets the HW (!) Device Specific info
BOOL AudioHW::bGetDevSpecific()
{
    return m_bDevSpecific;
} 

// Sets the HW (!) Device Specific info
void AudioHW::bSetDevSpecific(IN BOOL bDevSpecific)
{
    m_bDevSpecific = bDevSpecific;
}

// Gets the HW (!) Device Specific info
INT AudioHW::iGetDevSpecific()
{
    return m_iDevSpecific;
}

/* Sets the HW (!) Device Specific info
   Arguments:  fDevSpecific - true or false */
void AudioHW::iSetDevSpecific(IN  INT iDevSpecific)
{
    m_iDevSpecific = iDevSpecific;
}

// Gets the HW (!) Device Specific info
UINT AudioHW::uiGetDevSpecific()
{
    return m_uiDevSpecific;
} // uiGetDevSpecific

// Sets the HW (!) Device Specific info
void AudioHW::uiSetDevSpecific (IN  UINT uiDevSpecific)
{
    m_uiDevSpecific = uiDevSpecific;
}

/*  Gets the HW (!) mute levels for MSVAD
    Arguments: ulNode - topology node id
    Return Value: mute setting */
BOOL AudioHW::GetMixerMute(IN  ULONG ulNode)
{
    if (ulNode < MAX_TOPOLOGY_NODES)
    {
        return m_MuteControls[ulNode];
    }

    return 0;
}

// Return the current mux selection
ULONG AudioHW::GetMixerMux()
{
    return m_ulMux;
}

/*  Gets the HW (!) volume for MSVAD.
Arguments:  ulNode -   topology node id
            lChannel - which channel are we setting?
Return Value:  volume level */
LONG AudioHW::GetMixerVolume(IN ULONG ulNode,
                                    IN  LONG lChannel)
{
	UNREFERENCED_PARAMETER(lChannel);
    if (ulNode < MAX_TOPOLOGY_NODES)
    {
        return m_VolumeControls[ulNode];
    }

    return 0;
}

#pragma code_seg("PAGE")
// Resets the mixer registers.
void AudioHW::MixerReset()
{
    PAGED_CODE();
    
    RtlFillMemory(m_VolumeControls, sizeof(LONG) * MAX_TOPOLOGY_NODES, 0xFF);
    RtlFillMemory(m_MuteControls, sizeof(BOOL) * MAX_TOPOLOGY_NODES, TRUE);
    
    // BUGBUG change this depending on the topology
    m_ulMux = 2;
}
#pragma code_seg()

/* Sets the HW (!) mute levels for MSVAD
   Arguments: ulNode - topology node id
              fMute - mute flag */
void AudioHW::SetMixerMute(IN ULONG ulNode, IN  BOOL fMute)
{
    if (ulNode < MAX_TOPOLOGY_NODES)
    {
        m_MuteControls[ulNode] = fMute;
    }
} // SetMixerMute

/*  Sets the HW (!) mux selection
Arguments:  ulNode - topology node id */
void AudioHW::SetMixerMux(IN ULONG ulNode)
{
    m_ulMux = ulNode;
}

/* Sets the HW (!) volume for MSVAD.
  Arguments:  ulNode - topology node id
              lChannel - which channel are we setting?
              lVolume - volume level */
void AudioHW::SetMixerVolume (IN  ULONG ulNode,
                  IN  LONG lChannel, IN LONG lVolume)
{
	UNREFERENCED_PARAMETER(lChannel);
    if (ulNode < MAX_TOPOLOGY_NODES)
    {
        m_VolumeControls[ulNode] = lVolume;
    }
}
