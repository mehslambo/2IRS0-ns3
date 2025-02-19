/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// !!!!!!!!!!!!!!!!!!!!!!!
// Simulation parameters line 46~54
// configureTCPSensorClients is the only interesting function
// Node placement starts at line 892
// Channel construction starts at line 920
// Other functions are module creator/HaLow related functions, most interesting stuff is in TCPSensorClient.cc



#include "s1g-test-tim-raw.h"
#include "ns3/WUSN-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-mac-header.h"
#include <cstring>
#include <string.h>

NS_LOG_COMPONENT_DEFINE("soil");

uint32_t AssocNum = 0;
int64_t AssocTime = 0;
uint32_t StaNum = 0;
NetDeviceContainer staDeviceCont;
const int MaxSta = 8000;

Configuration config;
Statistics stats;

// ******
// SIMULATION PARAMS
// ******

Time stopTime = Hours(2);

uint32_t gateways = 1;
uint32_t totalNodes = 200;
uint32_t perAxis = 15;

uint32_t packetSize = 1;
int32_t totalSize = 1;

double areaSize = 12.0;
double depth = 0.3;

uint32_t simTime = 3600;
uint32_t timeOut = 10;

class assoc_record {
public:
	assoc_record();
	bool GetAssoc();
	void SetAssoc(std::string context, Mac48Address address);
	void UnsetAssoc(std::string context, Mac48Address address);
	void setstaid(uint16_t id);
        uint16_t getstaid();
private:
	bool assoc;
	uint16_t staid;
};

assoc_record::assoc_record() {
	assoc = false;
	staid = 65535;
}

void assoc_record::setstaid(uint16_t id) {
	staid = id;
}

uint16_t assoc_record::getstaid() {
	return staid;
}

void assoc_record::SetAssoc(std::string context, Mac48Address address) {
	assoc = true;
}

void assoc_record::UnsetAssoc(std::string context, Mac48Address address) {
	assoc = false;
}

bool assoc_record::GetAssoc() {
	return assoc;
}

typedef std::vector<assoc_record *> assoc_recordVector;
assoc_recordVector assoc_vector;

uint32_t GetAssocNum() {
	AssocNum = 0;
	for (assoc_recordVector::const_iterator index = assoc_vector.begin();
			index != assoc_vector.end(); index++) {
		if ((*index)->GetAssoc()) {
			AssocNum++;
		}
	}
	return AssocNum;
}

bool IsAssoc(uint16_t staid)
{
	for (assoc_recordVector::const_iterator index = assoc_vector.begin();
			index != assoc_vector.end(); index++) {
          if ((*index)->getstaid() == staid) {
            return ((*index)->GetAssoc());
          }
	}
	return false;
}


void PopulateArpCache() {
	Ptr<ArpCache> arp = CreateObject<ArpCache>();
	arp->SetAliveTimeout(Seconds(3600 * 24 * 365));
	for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
		Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol>();
		NS_ASSERT(ip != 0);
		ObjectVectorValue interfaces;
		ip->GetAttribute("InterfaceList", interfaces);
		for (ObjectVectorValue::Iterator j = interfaces.Begin();
				j != interfaces.End(); j++) {
			Ptr<Ipv4Interface> ipIface =
					(j->second)->GetObject<Ipv4Interface>();
			NS_ASSERT(ipIface != 0);
			Ptr<NetDevice> device = ipIface->GetDevice();
			NS_ASSERT(device != 0);
			Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress());
			for (uint32_t k = 0; k < ipIface->GetNAddresses(); k++) {
				Ipv4Address ipAddr = ipIface->GetAddress(k).GetLocal();
				if (ipAddr == Ipv4Address::GetLoopback())
					continue;
				ArpCache::Entry * entry = arp->Add(ipAddr);
				entry->MarkWaitReply(0);
				entry->MarkAlive(addr);
				std::cout << "Arp Cache: Adding the pair (" << addr << ","
						<< ipAddr << ")" << std::endl;
			}
		}
	}
	for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
		Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol>();
		NS_ASSERT(ip != 0);
		ObjectVectorValue interfaces;
		ip->GetAttribute("InterfaceList", interfaces);
		for (ObjectVectorValue::Iterator j = interfaces.Begin();
				j != interfaces.End(); j++) {
			Ptr<Ipv4Interface> ipIface =
					(j->second)->GetObject<Ipv4Interface>();
			ipIface->SetAttribute("ArpCache", PointerValue(arp));
		}
	}
}

uint16_t ngroup;
uint16_t nslot;
RPSVector configureRAW(RPSVector rpslist, string RAWConfigFile) {
	uint16_t NRPS = 0;
	uint16_t NRAWPERBEACON = 0;
	uint16_t Value = 0;
	uint32_t page = 0;
	uint32_t aid_start = 0;
	uint32_t aid_end = 0;
	uint32_t rawinfo = 0;

	ifstream myfile(RAWConfigFile);
	//1. get info from config file

	//2. define RPS
	if (myfile.is_open()) {
		myfile >> NRPS;
		int totalNumSta = 0;
		for (uint16_t kk = 0; kk < NRPS; kk++) // number of beacons covering all raw groups
		{
			RPS *m_rps = new RPS;
			myfile >> NRAWPERBEACON;
			ngroup = NRAWPERBEACON;
			for (uint16_t i = 0; i < NRAWPERBEACON; i++) // raw groups in one beacon
			{
				//RPS *m_rps = new RPS;
				RPS::RawAssignment *m_raw = new RPS::RawAssignment;

				myfile >> Value;
				m_raw->SetRawControl(Value);  //support paged STA or not
				myfile >> Value;
				m_raw->SetSlotCrossBoundary(Value);
				myfile >> Value;
				m_raw->SetSlotFormat(Value);
				myfile >> Value;
				m_raw->SetSlotDurationCount(Value);
				myfile >> Value;
				nslot = Value;
				m_raw->SetSlotNum(Value);
				myfile >> page;
				myfile >> aid_start;
				myfile >> aid_end;
				rawinfo = (aid_end << 13) | (aid_start << 2) | page;
				m_raw->SetRawGroup(rawinfo);
				totalNumSta += aid_end - aid_start + 1;
				m_rps->SetRawAssignment(*m_raw);
				delete m_raw;
			}
			rpslist.rpsset.push_back(m_rps);
			//config.nRawGroupsPerRpsList.push_back(NRAWPERBEACON);
		}
		myfile.close();
		config.NRawSta = totalNumSta;
				/*rpslist.rpsset[rpslist.rpsset.size() - 1]->GetRawAssigmentObj(
						NRAWPERBEACON - 1).GetRawGroupAIDEnd();*/
	} else
		cout << "Unable to open RAW configuration file \n";

	return rpslist;
}

/*
pageslice element and TIM(DTIM) together accomplish page slicing.

Prior knowledge:
802.11ah support up to 8192 stations, they are constructed into: page, block,
 subblock, sta.
there are 13 bit represent the AID of stations.
 AID[11-12] represent page.
 AID[6-10] represent block.
 AID[3-5] represent subblock.
 AID[0-2] represent sta.

A TIM(DTIM) element only support one page
A Page slice element only support one page

 Concept of page slicing:
 Between two DTIM beacon, there are many TIM beacons, only allow a TIM beacon include some blocks of one page is called page slice. One TIM beacon is called a page slice.
 Page slcie element specify number of page slice between two DTIM, number of blocks in each
 page slice.
 Page slice element only appears together with DTIM.

 Details:
 Page slice element also indicates AP has buffered data for which block, if a station is in that block, the station should first sleep, then wake up at coresponding page slice(TIM beacon) which includes that block.

 When station wake up at that block, it check whether AP has data for itself. If has, keep awake to receive packets and go to sleep in the next beacon.
 */

void configurePageSlice (void)
{
    config.pageS.SetPageindex (config.pageIndex);
    config.pageS.SetPagePeriod (config.pagePeriod); //2 TIM groups between DTIMs
    config.pageS.SetPageSliceLen (config.pageSliceLength); //each TIM group has 1 block (2 blocks in 2 TIM groups)
    config.pageS.SetPageSliceCount (config.pageSliceCount);
    config.pageS.SetBlockOffset (config.blockOffset);
    config.pageS.SetTIMOffset (config.timOffset);
    //std::cout << "pageIndex=" << (int)config.pageIndex << ", pagePeriod=" << (int)config.pagePeriod << ", pageSliceLength=" << (int)config.pageSliceLength << ", pageSliceCount=" << (int)config.pageSliceCount << ", blockOffset=" << (int)config.blockOffset << ", timOffset=" << (int)config.timOffset << std::endl;
    // page 0
    // 8 TIM(page slice) for one page
    // 4 block (each page)
    // 8 page slice
    // both offset are 0
}

void configureTIM (void)
{
    config.tim.SetPageIndex (config.pageIndex);
    if (config.pageSliceCount)
    	config.tim.SetDTIMPeriod (config.pageSliceCount); // not necessarily the same
    else
    	config.tim.SetDTIMPeriod (1);

    //std::cout << "DTIM period=" << (int)config.pagePeriod << std::endl;
}

void checkRawAndTimConfiguration (void)
{
	std::cout << "Checking RAW and TIM configuration..." << std::endl;
	bool configIsCorrect = true;
	NS_ASSERT (config.rps.rpsset.size());
	// Number of page slices in a single page has to equal number of different RPS elements because
	// If #PS > #RPS, the same RPS will be used in more than 1 PS and that is wrong because
	// each PS can accommodate different AIDs (same RPS means same stations in RAWs)
    if(config.pageSliceCount)
    {
	//NS_ASSERT (config.pagePeriod == config.rps.rpsset.size());
    }
	for (uint32_t j = 0; j < config.rps.rpsset.size(); j++)
	{
		uint32_t totalRawTime = 0;
		for (uint32_t i = 0; i < config.rps.rpsset[j]->GetNumberOfRawGroups(); i++)
		{
			totalRawTime += (120 * config.rps.rpsset[j]->GetRawAssigmentObj(i).GetSlotDurationCount() + 500) * config.rps.rpsset[j]->GetRawAssigmentObj(i).GetSlotNum();
			auto aidStart = config.rps.rpsset[j]->GetRawAssigmentObj(i).GetRawGroupAIDStart();
			auto aidEnd = config.rps.rpsset[j]->GetRawAssigmentObj(i).GetRawGroupAIDEnd();
			configIsCorrect = check (aidStart, j) && check (aidEnd, j);
			// AIDs in each RPS must comply with TIM in the following way:
			// TIM0: 1-63; TIM1: 64-127; TIM2: 128-191; ...; TIM32: 1983-2047
			// If RPS that belongs to TIM0 includes other AIDs (other than range [1-63]) configuration is incorrect
			NS_ASSERT (configIsCorrect);
		}
		NS_ASSERT (totalRawTime <= config.BeaconInterval);
	}
}
// assumes each TIM has its own beacon - doesn't need to be the case as there has to be only PageSliceCount beacons between DTIMs
bool check (uint16_t aid, uint32_t index)
{
	uint8_t block = (aid >> 6 ) & 0x001f;
	NS_ASSERT (config.pageS.GetPageSliceLen() > 0);
	//uint8_t toTim = (block - config.pageS.GetBlockOffset()) % config.pageS.GetPageSliceLen();
	if ((index == (uint32_t)(config.pageS.GetPageSliceCount() - 1)) && (config.pageS.GetPageSliceCount() != 0))
	{
		// the last page slice has 32 - the rest blocks
		return (block <= 31) && (block >= index * config.pageS.GetPageSliceLen());
	}
	else if (config.pageS.GetPageSliceCount() == 0)
		return true;

	return (block >= index * config.pageS.GetPageSliceLen()) && (block < (index + 1) * config.pageS.GetPageSliceLen());
}


void sendStatistics(bool schedule) {
	// reset
	std::fill(transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.begin(),
			transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.end(), 0);
	std::fill(transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.begin(),
			transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.end(), 0);

	if (schedule)
		Simulator::Schedule(Seconds(config.visualizerSamplingInterval),	&sendStatistics, true);
}

void onSTADeassociated(int i) {
}

void updateNodesQueueLength() {
	for (uint32_t i = 0; i < totalNodes; i++) {
		nodes[i]->UpdateQueueLength();
		stats.get(i).EDCAQueueLength = nodes[i]->queueLength;
	}
	Simulator::Schedule(Seconds(0.5), &updateNodesQueueLength);
}

static bool startedSending=false;

void CourseChangeCallback(std::string context, Ptr<const MobilityModel> mobility){
	//Print that we are logging the positions
	std::cout << "Logging node positions" << std::endl;
	std::string filePath = "postprocessing/logs/soil/course_change.csv";
	ofstream logFile(filePath,fstream::out | fstream::app);
	if(!logFile.is_open()){
		std::cout<<"Error opening file for logging node positions: "<<strerror(errno)<<std::endl;
		return;
	}

    Vector position = mobility->GetPosition ();
      
	// Identify if this node is a station or AP by checking its WifiNetDevice type.
	Ptr<Node> node = mobility->GetObject<Node>();
	bool isAp = false;

	// Look through all devices on the node:
	for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
	  Ptr<WifiNetDevice> wifiDev = node->GetDevice(i)->GetObject<WifiNetDevice>();
	  if (wifiDev) {
		// If its Mac is "ApWifiMac," it's an AP; otherwise it's a station.
		Ptr<ApWifiMac> apMac = wifiDev->GetMac()->GetObject<ApWifiMac>();
		if (apMac) {
		  isAp = true;
		  break;
		}
	  }
	}

	std::string nodeType = isAp ? "AP" : "STA";

	logFile
	  << Simulator::Now().GetNanoSeconds() << ";"
	  << context << ";"
	  << nodeType << ";"
	  << node->GetId() << ";"
	  << position.x << ";"
	  << position.y << ";"
	  << position.z << std::endl;
	// append to file
	logFile.close();
}

void AssocTimeoutStartSending()
{
  if (!startedSending) {
        startedSending=true;
	configureTCPSensorServer();
	configureTCPSensorClients();
	updateNodesQueueLength();
  }
}

void onSTAAssociated(int i) {
	cout << "Node " << std::to_string(i) << " is associated and has aid "
			<< nodes[i]->aId << endl;

	for (uint32_t k = 0; k < config.rps.rpsset.size(); k++) {
		for (uint32_t j = 0; j < config.rps.rpsset[k]->GetNumberOfRawGroups(); j++) {
			if (config.rps.rpsset[k]->GetRawAssigmentObj(j).GetRawGroupAIDStart()
					<= i + 1
					&& i + 1
							<= config.rps.rpsset[k]->GetRawAssigmentObj(j).GetRawGroupAIDEnd()) {
				nodes[i]->rpsIndex = k + 1;
				nodes[i]->rawGroupNumber = j + 1;
				nodes[i]->rawSlotIndex =
						nodes[i]->aId
								% config.rps.rpsset[k]->GetRawAssigmentObj(j).GetSlotNum()
								+ 1;
				/*cout << "Node " << i << " with AID " << (int)nodes[i]->aId << " belongs to " << (int)nodes[i]->rawSlotIndex << " slot of RAW group "
				 << (int)nodes[i]->rawGroupNumber << " within the " << (int)nodes[i]->rpsIndex << " RPS." << endl;
				 */
			}
		}
	}


	// RPS, Raw group and RAW slot assignment

	if (GetAssocNum() == (totalNodes)) {
		cout << "All " << AssocNum << " stations associated at " << Simulator::Now ().GetMicroSeconds () <<", configuring clients & server" << endl;

		// association complete, start sending packets
			stats.TimeWhenEverySTAIsAssociated = Simulator::Now();

                        Simulator::Schedule(Seconds(0.5), &AssocTimeoutStartSending);
	}
}

void RpsIndexTrace(uint16_t oldValue, uint16_t newValue) {
	currentRps = newValue;
	//cout << "RPS: " << newValue << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void RawGroupTrace(uint8_t oldValue, uint8_t newValue) {
	currentRawGroup = newValue;
	//cout << "	group " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void RawSlotTrace(uint8_t oldValue, uint8_t newValue) {
	currentRawSlot = newValue;
	//cout << "		slot " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void configureNodes(NodeContainer& wifiStaNode, NetDeviceContainer& staDevice) {
	cout << "Configuring STA Node trace sources..." << endl;

	for (uint32_t i = 0; i < (totalNodes); i++) {

		NodeEntry* n = new NodeEntry(i, &stats, wifiStaNode.Get(i),
				staDevice.Get(i));

		n->SetAssociatedCallback([ = ] {onSTAAssociated(i);});
		n->SetDeassociatedCallback([ = ] {onSTADeassociated(i);});

		nodes.push_back(n);
		// hook up Associated and Deassociated events
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc",
				MakeCallback(&NodeEntry::SetAssociation, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc",
				MakeCallback(&NodeEntry::UnsetAssociation, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/NrOfTransmissionsDuringRAWSlot",
				MakeCallback(
						&NodeEntry::OnNrOfTransmissionsDuringRAWSlotChanged,
						n));	//not implem

		//Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/S1gBeaconMissed", MakeCallback(&NodeEntry::OnS1gBeaconMissed, n));

		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/PacketDropped",
				MakeCallback(&NodeEntry::OnMacPacketDropped, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Collision",
				MakeCallback(&NodeEntry::OnCollision, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/TransmissionWillCrossRAWBoundary",
				MakeCallback(&NodeEntry::OnTransmissionWillCrossRAWBoundary,
						n)); //?

		// hook up TX
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxBegin",
				MakeCallback(&NodeEntry::OnPhyTxBegin, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd",
				MakeCallback(&NodeEntry::OnPhyTxEnd, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxDropWithReason",
				MakeCallback(&NodeEntry::OnPhyTxDrop, n)); //?

		// hook up RX
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxBegin",
				MakeCallback(&NodeEntry::OnPhyRxBegin, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxEnd",
				MakeCallback(&NodeEntry::OnPhyRxEnd, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason",
				MakeCallback(&NodeEntry::OnPhyRxDrop, n));

		// hook up MAC traces
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxRtsFailed",
				MakeCallback(&NodeEntry::OnMacTxRtsFailed, n)); //?
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxDataFailed",
				MakeCallback(&NodeEntry::OnMacTxDataFailed, n));
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxFinalRtsFailed",
				MakeCallback(&NodeEntry::OnMacTxFinalRtsFailed, n)); //?
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxFinalDataFailed",
				MakeCallback(&NodeEntry::OnMacTxFinalDataFailed, n)); //?

		// hook up PHY State change
		Config::Connect(
				"/NodeList/" + std::to_string(i)
						+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/State/State",
				MakeCallback(&NodeEntry::OnPhyStateChange, n));
	}
}


void OnAPPhyRxDrop(std::string context, Ptr<const Packet> packet,
		DropReason reason) {
	// THIS REQUIRES PACKET METADATA ENABLE!
	auto pCopy = packet->Copy();
	auto it = pCopy->BeginItem();
	while (it.HasNext()) {

		auto item = it.Next();
		Callback<ObjectBase *> constructor = item.tid.GetConstructor();

		ObjectBase *instance = constructor();
		Chunk *chunk = dynamic_cast<Chunk *>(instance);
		chunk->Deserialize(item.current);

		if (dynamic_cast<WifiMacHeader*>(chunk)) {
			WifiMacHeader* hdr = (WifiMacHeader*) chunk;

			int staId = -1;
			if (!config.useV6) {
				for (uint32_t i = 0; i < staNodeInterface.GetN(); i++) {
					if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress()
							== hdr->GetAddr2()) {
						staId = i;
						break;
					}
				}
			} else {
				for (uint32_t i = 0; i < staNodeInterface6.GetN(); i++) {
					if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress()
							== hdr->GetAddr2()) {
						staId = i;
						break;
					}
				}
			}
			if (staId != -1) {
				stats.get(staId).NumberOfDropsByReasonAtAP[reason]++;
			}
			delete chunk;
			break;
		} else
			delete chunk;
	}

}

void OnAPPacketToTransmitReceived(string context, Ptr<const Packet> packet,
		Mac48Address to, bool isScheduled, bool isDuringSlotOfSTA,
		Time timeLeftInSlot) {
	int staId = -1;
	if (!config.useV6) {
		for (uint32_t i = 0; i < staNodeInterface.GetN(); i++) {
			if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == to) {
				staId = i;
				break;
			}
		}
	} else {
		for (uint32_t i = 0; i < staNodeInterface6.GetN(); i++) {
			if (wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == to) {
				staId = i;
				break;
			}
		}
	}
	if (staId != -1) {
		if (isScheduled)
			stats.get(staId).NumberOfAPScheduledPacketForNodeInNextSlot++;
		else {
			stats.get(staId).NumberOfAPSentPacketForNodeImmediately++;
			stats.get(staId).APTotalTimeRemainingWhenSendingPacketInSameSlot +=
					timeLeftInSlot;
		}
	}
}

void onChannelTransmission(Ptr<NetDevice> senderDevice, Ptr<Packet> packet) {
	int rpsIndex = currentRps - 1;
	int rawGroup = currentRawGroup - 1;
	int slotIndex = currentRawSlot - 1;
	//cout << rpsIndex << "		" << rawGroup << "		" << slotIndex << "		" << endl;

	uint64_t iSlot = slotIndex;
	if (rpsIndex > 0)
		for (int r = rpsIndex - 1; r >= 0; r--)
			for (int g = 0; g < config.rps.rpsset[r]->GetNumberOfRawGroups(); g++)
				iSlot += config.rps.rpsset[r]->GetRawAssigmentObj(g).GetSlotNum();

	if (rawGroup > 0)
		for (int i = rawGroup - 1; i >= 0; i--)
			iSlot += config.rps.rpsset[rpsIndex]->GetRawAssigmentObj(i).GetSlotNum();

	if (rpsIndex >= 0 && rawGroup >= 0 && slotIndex >= 0)
	{
		if (senderDevice->GetAddress() == apDevice.Get(0)->GetAddress())
		{
			// from AP
			transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval[iSlot] += packet->GetSerializedSize();
		}
		else
		{
			// from STA
			transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval[iSlot] += packet->GetSerializedSize();

		}
	}
	//std::cout << "------------- packetSerializedSize = " << packet->GetSerializedSize() << std::endl;
	//std::cout << "------------- txAP[" << iSlot <<"] = " << transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval[iSlot] << std::endl;
	//std::cout << "------------- txSTA[" << iSlot <<"] = " << transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval[iSlot] << std::endl;

}

int getSTAIdFromAddress(Ipv4Address from) {
	int staId = -1;
	for (uint32_t i = 0; i < staNodeInterface.GetN(); i++) {
		if (staNodeInterface.GetAddress(i) == from) {
			staId = i;
			break;
		}
	}
	return staId;
}

void udpPacketReceivedAtServer(Ptr<const Packet> packet, Address from) { //works
	//cout << "+++++++++++udpPacketReceivedAtServer" << endl;
	int staId = getSTAIdFromAddress(
			InetSocketAddress::ConvertFrom(from).GetIpv4());
	if (staId != -1)
		nodes[staId]->OnUdpPacketReceivedAtAP(packet);
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;
}

void tcpPacketReceivedAtServer(Ptr<const Packet> packet, Address from) { 
	int staId = getSTAIdFromAddress(
			InetSocketAddress::ConvertFrom(from).GetIpv4());
	if (staId != -1)
		nodes[staId]->OnTcpPacketReceivedAtAP(packet);
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;
}

void tcpRetransmissionAtServer(Address to) {
	int staId = getSTAIdFromAddress(Ipv4Address::ConvertFrom(to));
	if (staId != -1)
	{
		nodes[staId]->OnTcpRetransmissionAtAP();
	}
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;
}

void tcpPacketDroppedAtServer(Address to, Ptr<Packet> packet,
		DropReason reason) {
	int staId = getSTAIdFromAddress(Ipv4Address::ConvertFrom(to));
	if (staId != -1) {
		stats.get(staId).NumberOfDropsByReasonAtAP[reason]++;
	}
}

void tcpStateChangeAtServer(TcpSocket::TcpStates_t oldState,
		TcpSocket::TcpStates_t newState, Address to) {

	int staId = getSTAIdFromAddress(
			InetSocketAddress::ConvertFrom(to).GetIpv4());
	if (staId != -1)
		nodes[staId]->OnTcpStateChangedAtAP(oldState, newState);
	else
		cout << "*** Node could not be determined from received packet at AP "
				<< endl;

	//cout << Simulator::Now().GetMicroSeconds() << " ********** TCP SERVER SOCKET STATE CHANGED FROM " << oldState << " TO " << newState << endl;
}




void configureTCPSensorServer() {
	ObjectFactory factory;
	factory.SetTypeId(TCPSensorServer::GetTypeId());
	factory.Set("Port", UintegerValue(84));

	Ptr<Application> tcpServer = factory.Create<TCPSensorServer>();
	wifiApNode.Get(0)->AddApplication(tcpServer);

	auto serverApp = ApplicationContainer(tcpServer);
	wireTCPServer(serverApp);
	serverApp.Start(Seconds(0));
	serverApp.Stop(stopTime);
}

void configureTCPSensorClients() {

	ObjectFactory factory;
	factory.SetTypeId(TCPSensorClient::GetTypeId());

	factory.Set("PacketSize", UintegerValue(packetSize));
	factory.Set("MeasurementSize", UintegerValue(totalSize));

	factory.Set("RemoteAddress",
			Ipv4AddressValue(apNodeInterface.GetAddress(0)));
	factory.Set("RemotePort", UintegerValue(84));

	Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();

	double itterator = 0;
	for (uint16_t i = 0; i < (totalNodes); i++) 
	{
          if (IsAssoc(i)) {
		factory.Set("id", UintegerValue(i));
		factory.Set("Interval", TimeValue(Minutes(i % 60)));
		Ptr<Application> tcpClient = factory.Create<TCPSensorClient>();
		wifiStaNode.Get(i)->AddApplication(tcpClient);
		auto clientApp = ApplicationContainer(tcpClient);
		wireTCPClient(clientApp, i);

		clientApp.Start(MilliSeconds(0));
		clientApp.Stop(stopTime);
		itterator += 0.25;
          } else {
            cout << "Not Associated: " << (int)i << endl;
          }
	}
}

void wireTCPServer(ApplicationContainer serverApp) {
	serverApp.Get(0)->TraceConnectWithoutContext("Rx",
			MakeCallback(&tcpPacketReceivedAtServer));
	serverApp.Get(0)->TraceConnectWithoutContext("Retransmission",
			MakeCallback(&tcpRetransmissionAtServer));
	serverApp.Get(0)->TraceConnectWithoutContext("PacketDropped",
			MakeCallback(&tcpPacketDroppedAtServer));
	serverApp.Get(0)->TraceConnectWithoutContext("TCPStateChanged",
			MakeCallback(&tcpStateChangeAtServer));

}

void wireTCPClient(ApplicationContainer clientApp, int i) {

	clientApp.Get(0)->TraceConnectWithoutContext("Tx",
			MakeCallback(&NodeEntry::OnTcpPacketSent, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("Rx",
			MakeCallback(&NodeEntry::OnTcpEchoPacketReceived, nodes[i]));

	clientApp.Get(0)->TraceConnectWithoutContext("CongestionWindow",
			MakeCallback(&NodeEntry::OnTcpCongestionWindowChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("RTO",
			MakeCallback(&NodeEntry::OnTcpRTOChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("RTT",
			MakeCallback(&NodeEntry::OnTcpRTTChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("SlowStartThreshold",
			MakeCallback(&NodeEntry::OnTcpSlowStartThresholdChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("EstimatedBW",
			MakeCallback(&NodeEntry::OnTcpEstimatedBWChanged, nodes[i]));

	clientApp.Get(0)->TraceConnectWithoutContext("TCPStateChanged",
			MakeCallback(&NodeEntry::OnTcpStateChanged, nodes[i]));
	clientApp.Get(0)->TraceConnectWithoutContext("Retransmission",
			MakeCallback(&NodeEntry::OnTcpRetransmission, nodes[i]));

	clientApp.Get(0)->TraceConnectWithoutContext("PacketDropped",
			MakeCallback(&NodeEntry::OnTcpPacketDropped, nodes[i]));

	if (config.trafficType == "tcpfirmware") {
		clientApp.Get(0)->TraceConnectWithoutContext("FirmwareUpdated",
				MakeCallback(&NodeEntry::OnTcpFirmwareUpdated, nodes[i]));
	} else if (config.trafficType == "tcpipcamera") {
		clientApp.Get(0)->TraceConnectWithoutContext("DataSent",
				MakeCallback(&NodeEntry::OnTcpIPCameraDataSent, nodes[i]));
		clientApp.Get(0)->TraceConnectWithoutContext("StreamStateChanged",
				MakeCallback(&NodeEntry::OnTcpIPCameraStreamStateChanged,
						nodes[i]));
	}
}


Time timeIdleArray[MaxSta];
Time timeRxArray[MaxSta];
Time timeTxArray[MaxSta];
Time timeSleepArray[MaxSta];
Time timeCollisionArray[MaxSta];

Time timeIdleNotAssociated[MaxSta];
Time timeRxNotAssociated[MaxSta];
Time timeTxNotAssociated[MaxSta];
Time timeSleepNotAssociated[MaxSta];
Time timeCollisionNotAssociated[MaxSta];

double dist[MaxSta];

//it prints the information regarding the state of the device
void PhyStateTrace(std::string context, Time start, Time duration,
		enum WifiPhy::State state) {

	/*Get the number of the node from the context*/
	/*context = "/NodeList/"+strSTA+"/DeviceList/'*'/Phy/$ns3::YansWifiPhy/State/State"*/
	unsigned first = context.find("t/");
	unsigned last = context.find("/D");
	string strNew = context.substr((first + 2), (last - first - 2));

	int node = std::stoi(strNew);

	if (nodes[node]->isAssociated)
	{
		switch (state)
		{
		case WifiPhy::State::SLEEP: //Sleep
			timeSleepArray[node] = timeSleepArray[node] + duration;
			break;
		case WifiPhy::State::IDLE: //Idle
			timeIdleArray[node] = timeIdleArray[node] + duration;
			break;
		case WifiPhy::State::TX: //Tx
			timeTxArray[node] = timeTxArray[node] + duration;
			break;
		case WifiPhy::State::RX: //Rx
			timeRxArray[node] = timeRxArray[node] + duration;
			break;
		case WifiPhy::State::CCA_BUSY: //CCA_BUSY
			timeCollisionArray[node] = timeCollisionArray[node] + duration;
			break;
		case WifiPhy::State::SWITCHING: //SWITCHING
			break;
		}
	}
	else
	{
		switch (state)
		{
		case WifiPhy::State::SLEEP: //Sleep
			timeSleepNotAssociated[node] = timeSleepNotAssociated[node] + duration;
			break;
		case WifiPhy::State::IDLE: //Idle
			timeIdleNotAssociated[node] = timeIdleNotAssociated[node] + duration;
			break;
		case WifiPhy::State::TX: //Tx
			timeTxNotAssociated[node] = timeTxNotAssociated[node] + duration;
			break;
		case WifiPhy::State::RX: //Rx
			timeRxNotAssociated[node] = timeRxNotAssociated[node] + duration;
			break;
		case WifiPhy::State::CCA_BUSY: //CCA_BUSY
			timeCollisionNotAssociated[node] = timeCollisionNotAssociated[node] + duration;
			break;
		case WifiPhy::State::SWITCHING: //SWITCHING
			break;
		}
	}
}

void MonitorSnifferRxCallback(std::string context, Ptr<const Packet> packet, 
	uint16_t channelFreqMhz, uint16_t channelNumber, 
	uint32_t rate, bool isShortPreamble, 
	WifiTxVector txVector,
	double signalDbm, double noiseDbm)
{
	//std::cout << "Logging packet receptions" << std::endl;
    // Extract node IDs from context
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t senderNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &senderNodeId);

    // Get the receiving node
    Ptr<Node> receiverNode = NodeList::GetNode(senderNodeId);
    uint32_t receiverNodeId = receiverNode->GetId();

    // Create a copy of the packet to read headers
    Ptr<Packet> copy = packet->Copy();
    WifiMacHeader wifiHeader;
    copy->RemoveHeader(wifiHeader);
    Mac48Address senderAddr = wifiHeader.GetAddr2();

	// Cast potentially problematic values to int to avoid null bytes
    int retries = static_cast<int>(txVector.GetRetries());
    int ness = static_cast<int>(txVector.GetNess());
    int nss = static_cast<int>(txVector.GetNss());
    int txPowerLevel = static_cast<int>(txVector.GetTxPowerLevel());

    // Log to file
    std::string filePath = "postprocessing/logs/soil/monitor_sniffer_rx.csv";
    std::ofstream logFile(filePath, std::ios::app);
	if(!logFile.is_open()){
		std::cout<<"Error opening file for logging node positions: "<<strerror(errno)<<std::endl;
		return;
	}
    logFile << Simulator::Now().GetSeconds() << ";"
	        << context << ";"
			<< channelFreqMhz << ";"
			<< channelNumber << ";"
			<< rate << ";"
			<< (isShortPreamble ? "true" : "false") << ";"
            << senderNodeId << ";"
            << receiverNodeId << ";"
			<< txVector.GetMode().GetUniqueName() << ";"
			<< retries << ";"
			<< ness << ";"
			<< nss << ";"
			<< (txVector.IsShortGuardInterval() ? "true" : "false") << ";"
			<< (txVector.IsStbc() ? "true" : "false") << ";"
			<< txPowerLevel << ";"
			<< noiseDbm << ";"
            << signalDbm << std::endl;
    logFile.close();
}

int main(int argc, char *argv[]) {
	//LogComponentEnable("TCPSensorServer", LOG_ALL);
	//LogComponentEnable("TCPSensorClient", LOG_ALL);
	LogComponentEnable("WUSN", LOG_ALL);

	// bool OutputPosition = true;

	CommandLine cmd;
	cmd.AddValue("sensors", "Number of sensors", totalNodes);
	cmd.AddValue("perAxis", "Sensors per axis", perAxis);
	cmd.AddValue("packetSize", "Size of sensor packet", packetSize);
	cmd.AddValue("totalSize", "Total bytes to be send", totalSize);
	// cmd.AddValue("areaSize", "Length of side of the area", areaSize);
	cmd.AddValue("areaSize", "Length of the side of the area", areaSize);
	cmd.AddValue("depth", "Depth below ground",depth);
        cmd.AddValue("stopTime", "Simulation time in seconds", simTime);
        cmd.AddValue("timeOut", "Association timeout in seconds", timeOut);
	// cmd.Parse(argc, argv);
	// CommandLine arguments are processed by Configuration.

	config = Configuration(&cmd, argc, argv);

        stopTime = Seconds(simTime);
	config.rps = configureRAW(config.rps, config.RAWConfigFile);

	configurePageSlice ();
	configureTIM ();
	//checkRawAndTimConfiguration ();

	// RV: the RAWConfig file assigns nodes to slots and groups.
	//     the number of nodes should match totalNodes.
	//     That is, use RAWGenerate to create a proper RAWConfig file.

	config.NSSFile = config.trafficType + "_" + std::to_string(totalNodes)
			+ "sta_" + std::to_string(config.NGroup) + "Group_"
			+ std::to_string(config.NRawSlotNum) + "slots_"
			+ std::to_string(config.payloadSize) + "payload_"
			+ std::to_string(config.totaltraffic) + "Mbps_"
			+ std::to_string(config.BeaconInterval) + "BI" + ".nss";

	stats = Statistics(totalNodes);
	uint32_t totalRawGroups(0);
	for (uint32_t i = 0; i < config.rps.rpsset.size(); i++) {
		int nRaw = config.rps.rpsset[i]->GetNumberOfRawGroups();
		totalRawGroups += nRaw;
		//cout << "Total raw groups after rps " << i << " is " << totalRawGroups << endl;
		for (int j = 0; j < nRaw; j++) {
			config.totalRawSlots += config.rps.rpsset[i]->GetRawAssigmentObj(j).GetSlotNum();
			//cout << "Total slots after group " << j << " is " << totalRawSlots << endl;
		}

	}
	transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval = vector<long>(
			config.totalRawSlots, 0);
	transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval = vector<long>(
			config.totalRawSlots, 0);








	wifiStaNode.Create(totalNodes);
	wifiApNode.Create(gateways * gateways);


	// **********************
	//  mobility.
	// **********************
	MobilityHelper mobilitySta;
	mobilitySta.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilitySta.Install(wifiStaNode);



	MobilityHelper mobilityAp;
	mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityAp.Install(wifiApNode);

    // Install the logger for the node positions whenever they change
	Config::Connect(
		"/NodeList/*/$ns3::MobilityModel/CourseChange",
		MakeCallback(&CourseChangeCallback)
	);

	// Make it so that nodes are at a certain height > 0
	// RV: the nodes are assigned to certain RAW groups and slots.
	//     the location of nodes might affect that assignment.
	//     that is, RAW groups could be used to avoid hidden node issues.
  int counter = 0;
  for (NodeContainer::Iterator j = wifiStaNode.Begin (); j != wifiStaNode.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
    	double dist = 1.0 * areaSize / perAxis;
       double x = 0.5 * dist + dist * (counter % perAxis);
        double y = 0.5*dist + dist * int(counter / perAxis);

      Vector position = mobility->GetPosition ();
      position.x = x;
      position.y = y;
      position.z = -(depth);
      mobility->SetPosition (position);
      
      counter++;

	  // Log to 
    }


  for (uint32_t i = 0; i < gateways * gateways; i++)
  {
    //int perAxis = gateways;
    //double dist = 5049 / perAxis;
    //double x = dist * (i % perAxis);
    //double y = dist * int(i / perAxis);
    wifiApNode.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(areaSize/2.0,areaSize/2.0,2.0));
  }


	// *************
	// CHANNEL 
	// ************

	Ptr<WUSNLossModel> loss = CreateObject<WUSNLossModel>();
  	loss->frequency = 868000000;

  	Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  	Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel> ();
	  channel->SetPropagationLossModel(loss);
	  channel->SetPropagationDelayModel(delay);
	channel->TraceConnectWithoutContext("Transmission",
			MakeCallback(&onChannelTransmission)); //TODO

	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetErrorRateModel("ns3::YansErrorRateModel");
	phy.SetChannel(channel);
	phy.Set("ShortGuardEnabled", BooleanValue(false));
	phy.Set("ChannelWidth", UintegerValue(1)); // changed
	phy.Set("EnergyDetectionThreshold", DoubleValue(-130.0));
	phy.Set("CcaMode1Threshold", DoubleValue(-130.0));
	phy.Set("TxGain", DoubleValue(14.0));
	phy.Set("RxGain", DoubleValue(0.0));
	phy.Set("TxPowerLevels", UintegerValue(1));
	phy.Set("TxPowerEnd", DoubleValue(0.0));
	phy.Set("TxPowerStart", DoubleValue(0.0));
	phy.Set("RxNoiseFigure", DoubleValue(6.8));
	phy.Set("LdpcEnabled", BooleanValue(true));
	phy.Set("S1g1MfieldEnabled", BooleanValue(true));

	WifiHelper wifi = WifiHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD_80211ah);
	S1gWifiMacHelper mac = S1gWifiMacHelper::Default();

	Ssid ssid = Ssid("ns380211ah");
	StringValue DataRate;
	DataRate = StringValue("OfdmRate1_2MbpsBW1MHz"); // changed

	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", DataRate, "ControlMode", DataRate);


	mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing",
			BooleanValue(false));

	NetDeviceContainer staDevice;
	staDevice = wifi.Install(phy, mac, wifiStaNode);

	mac.SetType ("ns3::ApWifiMac",
	                 "Ssid", SsidValue (ssid),
	                 "BeaconInterval", TimeValue (MicroSeconds(config.BeaconInterval)),
	                 "NRawStations", UintegerValue (config.NRawSta),
	                 "RPSsetup", RPSVectorValue (config.rps),
	                 "PageSliceSet", pageSliceValue (config.pageS),
	                 "TIMSet", TIMValue (config.tim)
	               );

	phy.Set("TxGain", DoubleValue(3.0));
	phy.Set("RxGain", DoubleValue(3.0));
	phy.Set("TxPowerLevels", UintegerValue(1));
	phy.Set("TxPowerEnd", DoubleValue(30.0));
	phy.Set("TxPowerStart", DoubleValue(30.0));
	phy.Set("RxNoiseFigure", DoubleValue(6.8));

	apDevice = wifi.Install(phy, mac, wifiApNode);

	Config::Set(
			"/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber",
			UintegerValue(10));
	Config::Set(
			"/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay",
			TimeValue(NanoSeconds(6000000000000)));

	std::ostringstream oss;
	oss << "/NodeList/" << wifiApNode.Get(0)->GetId()
			<< "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::ApWifiMac/";
	Config::ConnectWithoutContext(oss.str() + "RpsIndex", MakeCallback(&RpsIndexTrace));
	Config::ConnectWithoutContext(oss.str() + "RawGroup", MakeCallback(&RawGroupTrace));
	Config::ConnectWithoutContext(oss.str() + "RawSlot", MakeCallback(&RawSlotTrace));

    // Install the logger for transmission power
	Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
		MakeCallback(&MonitorSnifferRxCallback));









	/* Internet stack*/
	InternetStackHelper stack;
	stack.Install(wifiApNode);
	stack.Install(wifiStaNode);

	Ipv4AddressHelper address;

	address.SetBase("192.168.0.0", "255.255.0.0");

	staNodeInterface = address.Assign(staDevice);
	apNodeInterface = address.Assign(apDevice);

	//trace association
	std::cout << "Configuring trace sources..." << std::endl;
	for (uint16_t kk = 0; kk < (totalNodes); kk++) {
		std::ostringstream STA;
		STA << kk;
		std::string strSTA = STA.str();

		assoc_record *m_assocrecord = new assoc_record;
		m_assocrecord->setstaid(kk);
		Config::Connect(
				"/NodeList/" + strSTA
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc",
				MakeCallback(&assoc_record::SetAssoc, m_assocrecord));
		Config::Connect(
				"/NodeList/" + strSTA
						+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc",
				MakeCallback(&assoc_record::UnsetAssoc, m_assocrecord));
		assoc_vector.push_back(m_assocrecord);
	}
	

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	PopulateArpCache();
	configureNodes(wifiStaNode, staDevice);
	Config::Connect(
			"/NodeList/" + std::to_string(totalNodes)
					+ "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason",
			MakeCallback(&OnAPPhyRxDrop));
	Config::Connect(
			"/NodeList/" + std::to_string(totalNodes)
					+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/PacketToTransmitReceivedFromUpperLayer",
			MakeCallback(&OnAPPacketToTransmitReceived));


	// Force a position update to log the positions at T=0		
	Ptr<MobilityModel> mobility1 =
			wifiApNode.Get(0)->GetObject<MobilityModel>();
    Vector apposition = mobility1->GetPosition();
    mobility1->SetPosition(apposition);


	sendStatistics(false);



	// ******************
	//  SIMULATION
	// ******************
    Simulator::Schedule(Seconds(timeOut), &AssocTimeoutStartSending);

    

	Simulator::Stop(stopTime); // allow up to a minute after the client & server apps are finished to process the queue
	Simulator::Run();

	// Visualizer throughput
	int pay = 0, totalSuccessfulPackets = 0, totalSentPackets = 0, totalPacketsEchoed = 0;
	bool delivered = true;
	for (uint32_t i = 0; i < (totalNodes); i++)
	{
		if (stats.get(i).NumberOfSuccessfulPackets == 0)
		{
			delivered = false;
		}
		totalSuccessfulPackets += stats.get(i).NumberOfSuccessfulPackets;
		totalSentPackets += stats.get(i).NumberOfSentPackets;
		pay += stats.get(i).TotalPacketPayloadSize;
		cout << i << " sent: " << stats.get(i).NumberOfSentPackets
				<< " ; delivered: " << stats.get(i).NumberOfSuccessfulPackets
				<< "; retransmissions:" << stats.get(i).NumberOfTCPRetransmissions
				<< "; TotalPacketPayloadSize:" << stats.get(i).TotalPacketPayloadSize << std::endl;

	}

	if (!delivered)
	{
		cout << "missed" << std::endl;
	}

    cout << "total send " << totalSuccessfulPackets << " needed: 413" << std::endl;

	cout << "total packet loss % "
			<< 100 - 100. * totalPacketsEchoed / totalSentPackets << endl;
	Simulator::Destroy();

	return 0;
}
