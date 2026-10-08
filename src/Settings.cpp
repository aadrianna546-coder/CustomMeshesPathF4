#include "Settings.h"

namespace Settings
{
	bool debug = false;
	UInt32 actorLoadID = 0;
	UInt32 meshPathID = 0;

	void Load()
	{
		debug = GetPrivateProfileIntA("Debug", "bDebug", 0, kIniPath) != 0;
		actorLoadID = GetPrivateProfileIntA("Advanced", "iActorLoadID", 0, kIniPath);
		meshPathID = GetPrivateProfileIntA("Advanced", "iMeshPathID", 0, kIniPath);

		_MESSAGE("bDebug: %d", debug ? 1 : 0);
		if (actorLoadID || meshPathID) {
			_MESSAGE("iActorLoadID: %u, iMeshPathID: %u", actorLoadID, meshPathID);
		}
	}
}
