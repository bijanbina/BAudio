#ifndef __HW_H_
#define __HW_H_

// BUGBUG we should dynamically allocate this...
#define MAX_TOPOLOGY_NODES 20

// This class represents virtual HW. An array representing volume
// registers and mute registers.
class AudioHW 
{
	public:
		AudioHW();
		void MixerReset();
  
		BOOL bGetDevSpecific();
		void bSetDevSpecific(IN  BOOL bDevSpecific);
		INT  iGetDevSpecific();
		void iSetDevSpecific(IN  INT iDevSpecific);
		UINT uiGetDevSpecific();
		void uiSetDevSpecific(IN  UINT uiDevSpecific); 
  
		BOOL GetMixerMute(IN ULONG ulNode);
		void SetMixerMute(IN ULONG ulNode, IN BOOL fMute);

		ULONG GetMixerMux();
		void SetMixerMux(IN ULONG ulNode);

		LONG GetMixerVolume(IN ULONG ulNode, IN LONG lChannel);
		void SetMixerVolume(IN ULONG ulNode, IN LONG lChannel, IN LONG lVolume);

	protected:
		BOOL   m_MuteControls[MAX_TOPOLOGY_NODES];
		LONG   m_VolumeControls[MAX_TOPOLOGY_NODES];
		ULONG  m_ulMux;            // Mux selection
		BOOL   m_bDevSpecific;
		INT    m_iDevSpecific;
		UINT   m_uiDevSpecific;
};

typedef AudioHW *PAudioHW;

#endif
