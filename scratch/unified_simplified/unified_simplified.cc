/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

#include "models/underwater-propagation-loss-model.h"
#include "models/underwater-propagation-loss-model.cc"
#include "models/underwater-propagation-delay-model.h"
#include "models/underwater-propagation-delay-model.cc"

#include "Configuration.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UnifiedSimplified");

Configuration config;
NodeContainer apNodes;
NodeContainer staNodes;
NetDeviceContainer apDevices;
NetDeviceContainer staDevices;
Ipv4InterfaceContainer apInterfaces;
Ipv4InterfaceContainer staInterfaces;

vector<long> transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval;
vector<long> transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval;
uint16_t ngroup;
uint16_t nslot;
RPSVector configureRAW(RPSVector rpslist, string RAWConfigFile)
{
  uint16_t NRPS = 0;
  uint16_t NRAWPERBEACON = 0;
  uint16_t Value = 0;
  uint32_t page = 0;
  uint32_t aid_start = 0;
  uint32_t aid_end = 0;
  uint32_t rawinfo = 0;

  ifstream myfile(RAWConfigFile);
  // 1. get info from config file

  // 2. define RPS
  if (myfile.is_open())
  {
    myfile >> NRPS;
    int totalNumSta = 0;
    for (uint16_t kk = 0; kk < NRPS; kk++) // number of beacons covering all raw groups
    {
      RPS *m_rps = new RPS;
      myfile >> NRAWPERBEACON;
      ngroup = NRAWPERBEACON;
      for (uint16_t i = 0; i < NRAWPERBEACON; i++) // raw groups in one beacon
      {
        // RPS *m_rps = new RPS;
        RPS::RawAssignment *m_raw = new RPS::RawAssignment;

        myfile >> Value;
        m_raw->SetRawControl(Value); // support paged STA or not
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
      // config.nRawGroupsPerRpsList.push_back(NRAWPERBEACON);
    }
    myfile.close();
    config.NRawSta = totalNumSta;
    /*rpslist.rpsset[rpslist.rpsset.size() - 1]->GetRawAssigmentObj(
        NRAWPERBEACON - 1).GetRawGroupAIDEnd();*/
  }
  else
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

void configurePageSlice(void)
{
  config.pageS.SetPageindex(config.pageIndex);
  config.pageS.SetPagePeriod(config.pagePeriod);        // 2 TIM groups between DTIMs
  config.pageS.SetPageSliceLen(config.pageSliceLength); // each TIM group has 1 block (2 blocks in 2 TIM groups)
  config.pageS.SetPageSliceCount(config.pageSliceCount);
  config.pageS.SetBlockOffset(config.blockOffset);
  config.pageS.SetTIMOffset(config.timOffset);
  // std::cout << "pageIndex=" << (int)config.pageIndex << ", pagePeriod=" << (int)config.pagePeriod << ", pageSliceLength=" << (int)config.pageSliceLength << ", pageSliceCount=" << (int)config.pageSliceCount << ", blockOffset=" << (int)config.blockOffset << ", timOffset=" << (int)config.timOffset << std::endl;
  //  page 0
  //  8 TIM(page slice) for one page
  //  4 block (each page)
  //  8 page slice
  //  both offset are 0
}

void configureTIM(void)
{
  config.tim.SetPageIndex(config.pageIndex);
  if (config.pageSliceCount)
    config.tim.SetDTIMPeriod(config.pageSliceCount); // not necessarily the same
  else
    config.tim.SetDTIMPeriod(1);

  // std::cout << "DTIM period=" << (int)config.pagePeriod << std::endl;
}

// assumes each TIM has its own beacon - doesn't need to be the case as there has to be only PageSliceCount beacons between DTIMs
bool check(uint16_t aid, uint32_t index)
{
  uint8_t block = (aid >> 6) & 0x001f;
  NS_ASSERT(config.pageS.GetPageSliceLen() > 0);
  // uint8_t toTim = (block - config.pageS.GetBlockOffset()) % config.pageS.GetPageSliceLen();
  if ((index == (uint32_t)(config.pageS.GetPageSliceCount() - 1)) && (config.pageS.GetPageSliceCount() != 0))
  {
    // the last page slice has 32 - the rest blocks
    return (block <= 31) && (block >= index * config.pageS.GetPageSliceLen());
  }
  else if (config.pageS.GetPageSliceCount() == 0)
    return true;

  return (block >= index * config.pageS.GetPageSliceLen()) && (block < (index + 1) * config.pageS.GetPageSliceLen());
}

void checkRawAndTimConfiguration(void)
{
  std::cout << "Checking RAW and TIM configuration..." << std::endl;
  bool configIsCorrect = true;
  NS_ASSERT(config.rps.rpsset.size());
  // Number of page slices in a single page has to equal number of different RPS elements because
  // If #PS > #RPS, the same RPS will be used in more than 1 PS and that is wrong because
  // each PS can accommodate different AIDs (same RPS means same stations in RAWs)
  if (config.pageSliceCount)
  {
    // NS_ASSERT (config.pagePeriod == config.rps.rpsset.size());
  }
  for (uint32_t j = 0; j < config.rps.rpsset.size(); j++)
  {
    uint32_t totalRawTime = 0;
    for (uint32_t i = 0; i < config.rps.rpsset[j]->GetNumberOfRawGroups(); i++)
    {
      totalRawTime += (120 * config.rps.rpsset[j]->GetRawAssigmentObj(i).GetSlotDurationCount() + 500) * config.rps.rpsset[j]->GetRawAssigmentObj(i).GetSlotNum();
      auto aidStart = config.rps.rpsset[j]->GetRawAssigmentObj(i).GetRawGroupAIDStart();
      auto aidEnd = config.rps.rpsset[j]->GetRawAssigmentObj(i).GetRawGroupAIDEnd();
      configIsCorrect = check(aidStart, j) && check(aidEnd, j);
      // AIDs in each RPS must comply with TIM in the following way:
      // TIM0: 1-63; TIM1: 64-127; TIM2: 128-191; ...; TIM32: 1983-2047
      // If RPS that belongs to TIM0 includes other AIDs (other than range [1-63]) configuration is incorrect
      NS_ASSERT(configIsCorrect);
    }
    NS_ASSERT(totalRawTime <= config.BeaconInterval);
  }
}

uint16_t currentRps;
void RpsIndexTrace(uint16_t oldValue, uint16_t newValue)
{
  currentRps = newValue;
  // cout << "RPS: " << newValue << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

uint16_t currentRawGroup;
void RawGroupTrace(uint8_t oldValue, uint8_t newValue)
{
  currentRawGroup = newValue;
  // cout << "	group " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

uint16_t currentRawSlot;
void RawSlotTrace(uint8_t oldValue, uint8_t newValue)
{
  currentRawSlot = newValue;
  // cout << "		slot " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void onChannelTransmission(Ptr<NetDevice> senderDevice, Ptr<Packet> packet)
{
  int rpsIndex = currentRps - 1;
  int rawGroup = currentRawGroup - 1;
  int slotIndex = currentRawSlot - 1;
  // cout << rpsIndex << "		" << rawGroup << "		" << slotIndex << "		" << endl;

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
    if (senderDevice->GetAddress() == apDevices.Get(0)->GetAddress())
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
}

void onSTAAssociated(std::string context, ns3::Mac48Address address)
{
  NS_LOG_INFO("Station " << address << " was associated");
}

void onSTADeassociated(std::string context, ns3::Mac48Address address)
{
  NS_LOG_INFO("Station " << address << " was deassociated");
}

void setupAssociationTraces()
{
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/Assoc", MakeCallback(&onSTAAssociated));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/DeAssoc", MakeCallback(&onSTADeassociated));
}

bool IsAssoc(int staId) {
  Ptr<WifiNetDevice> staDev = DynamicCast<WifiNetDevice>(staDevices.Get(staId));
  Ptr<StaWifiMac> staMac = DynamicCast<StaWifiMac>(staDev->GetMac());
  if (staMac && staMac->GetBssid() != Mac48Address()) {
    return true;
  }
  return false;
}

void ReassociateIfNeeded()
{
  Ptr<WifiNetDevice> staDev = DynamicCast<WifiNetDevice>(staDevices.Get(0));
  Ptr<StaWifiMac> staMac = DynamicCast<StaWifiMac>(staDev->GetMac());
  if (staMac && staMac->GetBssid() == Mac48Address()) {
    NS_LOG_INFO("Forcing re-association attempt via StartActiveAssociation()");
    staMac->StartActiveAssociation();
  }
  Simulator::Schedule(Seconds(.1), &ReassociateIfNeeded);
}

void toggleAPPosition()
{
  static bool toggle = false;
  Ptr<MobilityModel> mob = apNodes.Get(0)->GetObject<MobilityModel>();
  if (toggle)
  {
    mob->SetPosition(Vector(1000, 1000, 0));
    std::cout << "AP moved to (1000,1000,0)" << std::endl;
  }
  else
  {
    mob->SetPosition(Vector(0, 0, 0));
    std::cout << "AP moved to (0,0,0)" << std::endl;
  }
  toggle = !toggle;
  Simulator::Schedule(Seconds(1), &toggleAPPosition);
}

int main(int argc, char *argv[])
{
  LogComponentEnable("UnifiedSimplified", LOG_ALL);
  CommandLine cmd;
  // cmd.Parse (argc, argv);
  config = Configuration(&cmd, argc, argv);

  std::ostringstream argStream;
  for (int i = 1; i < argc; i++)
  { // Skip argv[0] (program name)
    if (i > 1)
      argStream << "_"; // Add space between arguments

    // Sanitize the string to make it suitable for filenames
    std::string arg = argv[i];
    std::string cleanArg;
    for (char c : arg)
    {
      if (c != '-')
        cleanArg += c;
    }

    argStream << cleanArg;
  }

  config.rps = configureRAW(config.rps, config.RAWConfigFile);
  int totalNodes = 1;

  configurePageSlice();
  configureTIM();
  checkRawAndTimConfiguration ();

  // RV: the RAWConfig file assigns nodes to slots and groups.
  //     the number of nodes should match totalNodes.
  //     That is, use RAWGenerate to create a proper RAWConfig file.

  config.NSSFile = config.trafficType + "_" + std::to_string(totalNodes) + "sta_" + std::to_string(config.NGroup) + "Group_" + std::to_string(config.NRawSlotNum) + "slots_" + std::to_string(config.payloadSize) + "payload_" + std::to_string(config.totaltraffic) + "Mbps_" + std::to_string(config.BeaconInterval) + "BI" + ".nss";

  uint32_t totalRawGroups(0);
  for (uint32_t i = 0; i < config.rps.rpsset.size(); i++)
  {
    int nRaw = config.rps.rpsset[i]->GetNumberOfRawGroups();
    totalRawGroups += nRaw;
    // cout << "Total raw groups after rps " << i << " is " << totalRawGroups << endl;
    for (int j = 0; j < nRaw; j++)
    {
      config.totalRawSlots += config.rps.rpsset[i]->GetRawAssigmentObj(j).GetSlotNum();
      // cout << "Total slots after group " << j << " is " << totalRawSlots << endl;
    }
  }

  transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval = vector<long>(
      config.totalRawSlots, 0);
  transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval = vector<long>(
      config.totalRawSlots, 0);

  // Create nodes: one for the AP and one for the STA.
  NS_LOG_INFO("Creating nodes");
  apNodes.Create(1);
  staNodes.Create(1);

  std::string propagationModel = "freshwater";

  Ptr<FriisPropagationLossModel> friisLoss = CreateObject<FriisPropagationLossModel>();
  Ptr<UnderwaterPropagationLossModel> underwaterLoss = CreateObject<UnderwaterPropagationLossModel>();
  Ptr<UnderwaterPropagationDelayModel> underwaterDelay = CreateObject<UnderwaterPropagationDelayModel>();
  Ptr<PropagationDelayModel> constantDelay = CreateObject<ConstantSpeedPropagationDelayModel>();

  // Set up the WiFi channel and PHY layer.
  NS_LOG_INFO("Setting up the WiFi channel and PHY layer");
  Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
  if (propagationModel == "air")
  {
    std::cout << "Using Friis propagation loss model" << std::endl;
    channel->SetPropagationLossModel(friisLoss);
    std::cout << "Using constant propagation delay model" << std::endl;
    channel->SetPropagationDelayModel(constantDelay);
  }
  else if (propagationModel == "freshwater")
  {
    std::cout << "Using Friis + underwater propagation loss model" << std::endl;
    friisLoss->SetNext(underwaterLoss);
    channel->SetPropagationLossModel(friisLoss);
    std::cout << "Using underwater propagation delay model" << std::endl;
    channel->SetPropagationDelayModel(underwaterDelay);
  }
  else
  {
    std::cout << "Invalid propagation model specified" << std::endl;
    return 0;
  }
  channel->TraceConnectWithoutContext("Transmission",
                                      MakeCallback(&onChannelTransmission)); // TODO

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetErrorRateModel("ns3::YansErrorRateModel");
  phy.SetChannel(channel);
  phy.Set("ShortGuardEnabled", BooleanValue(true));
  phy.Set("ChannelWidth", UintegerValue(1)); // Only 1/2; 4 is unstable and not documented; 8/16MHz is not implemented :(
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

  // Configure the WifiHelper and set the standard to 802.11ah.
  NS_LOG_INFO("Configuring the WifiHelper");
  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211ah);
  // Use a constant rate manager with a fixed data mode, similar to the sample code.
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("OfdmRate1_2MbpsBW1MHz"),
                               "ControlMode", StringValue("OfdmRate1_2MbpsBW1MHz"));

  // Define the common SSID.
  Ssid ssid = Ssid("ns380211ah");

  // Use the S1g (802.11ah) MAC helper.
  S1gWifiMacHelper mac = S1gWifiMacHelper::Default();

  // Configure the STA: disable active probing.
  NS_LOG_INFO("Configuring the STA");
  mac.SetType("ns3::StaWifiMac",
              "Ssid", SsidValue(ssid),
              "ActiveProbing", BooleanValue(true));
  staDevices = wifi.Install(phy, mac, staNodes);

  // Configure the AP: set the beacon interval and the number of RAW stations (set to 1 here).
  NS_LOG_INFO("Configuring the AP");
  mac.SetType("ns3::ApWifiMac",
              "Ssid", SsidValue(ssid),
              "BeaconInterval", TimeValue(MicroSeconds(config.BeaconInterval)),
              "NRawStations", UintegerValue(config.NRawSta),
              "RPSsetup", RPSVectorValue(config.rps),
              "PageSliceSet", pageSliceValue(config.pageS),
              "TIMSet", TIMValue(config.tim));

  phy.Set("TxGain", DoubleValue(3.0));
  phy.Set("RxGain", DoubleValue(3.0));
  phy.Set("TxPowerLevels", UintegerValue(1));
  phy.Set("TxPowerEnd", DoubleValue(30.0));
  phy.Set("TxPowerStart", DoubleValue(30.0));
  phy.Set("RxNoiseFigure", DoubleValue(6.8));
  apDevices = wifi.Install(phy, mac, apNodes);

  Ptr<WifiNetDevice> wifiApNodeToGetFrequency = DynamicCast<WifiNetDevice>(apNodes.Get(0)->GetDevice(0));
  Ptr<WifiPhy> phyToGetFrequency = wifiApNodeToGetFrequency->GetPhy();
  double ApFrequencyHz = phyToGetFrequency->GetFrequency() * 1e6;
  std::cout << "AP operates at " << ApFrequencyHz << " Hz" << std::endl;

  underwaterLoss->SetAttribute("Frequency", DoubleValue(ApFrequencyHz));  // HaLow frequency based on what's seen in PHY logging
  underwaterDelay->SetAttribute("Frequency", DoubleValue(ApFrequencyHz)); // HaLow frequency based on what's seen in PHY logging

  Config::Set(
      "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber",
      UintegerValue(10));
  Config::Set(
      "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay",
      TimeValue(NanoSeconds(6000000000000)));

  std::ostringstream oss;
  oss << "/NodeList/" << apNodes.Get(0)->GetId()
      << "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::ApWifiMac/";
  Config::ConnectWithoutContext(oss.str() + "RpsIndex", MakeCallback(&RpsIndexTrace));
  Config::ConnectWithoutContext(oss.str() + "RawGroup", MakeCallback(&RawGroupTrace));
  Config::ConnectWithoutContext(oss.str() + "RawSlot", MakeCallback(&RawSlotTrace));

  setupAssociationTraces();
  Simulator::Schedule(Seconds(0), &ReassociateIfNeeded);

  // Set up mobility: place the AP at (0,0) and the STA at (10,0).
  NS_LOG_INFO("Setting up mobility");
  MobilityHelper mobilitySta;
  mobilitySta.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilitySta.Install(staNodes);

  Ptr<MobilityModel> mobSta = staNodes.Get(0)->GetObject<MobilityModel>();
  mobSta->SetPosition(Vector(-1, 0, 0));

  MobilityHelper mobilityAp;
  mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityAp.Install(apNodes);

  Ptr<MobilityModel> mobAp = apNodes.Get(0)->GetObject<MobilityModel>();
  mobAp->SetPosition(Vector(0, 0, 0));

  std::fill(transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.begin(),
            transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.end(), 0);
  std::fill(transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.begin(),
            transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.end(), 0);

  Simulator::Schedule(Seconds(1), &toggleAPPosition);

  // Install the Internet stack on both nodes.
  NS_LOG_INFO("Installing the Internet stack");
  InternetStackHelper stack;
  stack.Install(apNodes);
  stack.Install(staNodes);

  // Assign IP addresses.
  NS_LOG_INFO("Assigning IP addresses");
  Ipv4AddressHelper address;
  address.SetBase("192.168.1.0", "255.255.255.0");
  apInterfaces = address.Assign(apDevices);
  staInterfaces = address.Assign(staDevices);

  // Install a UDP server on the AP.
  NS_LOG_INFO("Installing a UDP server on the AP");
  uint16_t port = 9;
  UdpServerHelper server(port);
  ApplicationContainer serverApps = server.Install(apNodes.Get(0));
  serverApps.Start(Seconds(0.0));
  serverApps.Stop(Seconds(10.0));

  // Install a UDP client on the STA to send packets to the AP.
  NS_LOG_INFO("Installing a UDP client on the STA");
  UdpClientHelper client(apInterfaces.GetAddress(0), port);
  client.SetAttribute("MaxPackets", UintegerValue(100));
  client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  client.SetAttribute("PacketSize", UintegerValue(1024));
  NS_LOG_INFO("Installing the UDP client on the STA");
  ApplicationContainer clientApps = client.Install(staNodes.Get(0));
  clientApps.Start(Seconds(0.0));
  clientApps.Stop(Seconds(10.0));

  Simulator::Stop(Seconds(10.0));
  NS_LOG_INFO("Starting simulation");
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
