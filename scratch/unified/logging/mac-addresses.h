#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

// A simple struct to hold MAC addresses extracted from a Wifi header.
struct MacAddresses {
    Mac48Address receiver;
    Mac48Address transmitter;
    Mac48Address destination;
    Mac48Address source;
    Mac48Address bssid;
};