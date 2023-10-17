/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
  Author: Laura Michaella Batista Ribeiro <laura.michaella@gmail.com>
Basead on:
----  wave-simple-80211p example
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Junling Bu <linlinjavaer@gmail.com>
-----evalvid-client-server example from
 *  Author: Billy Pinheiro <haquiticos@gmail.com>
 */
/**
 * This example shows basic construction of an 802.11p node.  Two nodes
 * are constructed with 802.11p devices, and by default, one node sends a single
 * packet to another node (the number of packets and interval between
 * them can be configured by command-line arguments).  The example shows
 * typical usage of the helper classes for this mode of WiFi (where "OCB" refers
 * to "Outside the Context of a BSS")."
 ./waf --run "scratch/wave-simple-80211p-evalvid --mobility=1 --nodes=3 --totaltime=90 --packetSize=1472 --interval=0.1 --verbose=true" 2>&1 | tee testeEvalvid_80211p_verbose.txt
 */

#include "ns3/vector.h"
#include "ns3/string.h"
#include "ns3/socket.h"
#include "ns3/double.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/command-line.h"
#include "ns3/mobility-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/udp-client-server-helper.h"

#include <iostream>

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/netanim-module.h" //for the simulation

// for the video transmission
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-helper.h"
#include "ns3/evalvid-client-server-helper.h"
#include "ns3/wave-bsm-helper.h"
#include "ns3/mobility-module.h"



using namespace ns3;

// Global variables for use in callbacks.
double g_signalDbmAvg;
double g_noiseDbmAvg;
uint32_t g_samples;

void MonitorSniffRx (Ptr<const Packet> packet,
                     uint16_t channelFreqMhz,
                     WifiTxVector txVector,
                     MpduInfo aMpdu,
                     SignalNoiseDbm signalNoise)

{
  g_samples++;
  g_signalDbmAvg += ((signalNoise.signal - g_signalDbmAvg) / g_samples);
  g_noiseDbmAvg += ((signalNoise.noise - g_noiseDbmAvg) / g_samples);
}


NS_LOG_COMPONENT_DEFINE ("WaveEvalvid");

/*
 * In WAVE module, there is no net device class named like "Wifi80211pNetDevice",
 * instead, we need to use Wifi80211pHelper to create an object of
 * WifiNetDevice class.
 *
 * usage:
 *  NodeContainer nodes;
 *  NetDeviceContainer devices;
 *  nodes.Create (2);
 *  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
 *  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
 *  wifiPhy.SetChannel (wifiChannel.Create ());
 *  NqosWaveMacHelper wifi80211pMac = NqosWave80211pMacHelper::Default();
 *  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
 *  devices = wifi80211p.Install (wifiPhy, wifi80211pMac, nodes);
 *
 * The reason of not providing a 802.11p class is that most of modeling
 * 802.11p standard has been done in wifi module, so we only need a high
 * MAC class that enables OCB mode.
 */
  uint64_t pktCount_n;
void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                            uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      pktCount_n = pktCount;
      socket->Send (Create<Packet> (pktSize));
     Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
else
    {
      socket->Close ();
}
}

int main (int argc, char *argv[])
{
//
// Enable logging for EvalvidClient and
//
LogComponentEnable ("EvalvidClient", LOG_LEVEL_INFO);
LogComponentEnable ("EvalvidServer", LOG_LEVEL_INFO);

//Variables for spacing in the simulation (variables that depend on external libraries)
  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  std::string m_traceFile= "/home/stelios/mobility.tcl";
  std::string m_lossModelName;

  uint32_t m_mobility; ///< mobility
  uint32_t m_nNodes; ///< number of nodes
  uint32_t packetSize = 1472; // bytes
  uint32_t numPackets = 1;
  double interval = 1.0; // seconds
  bool verbose = false;
  double m_TotalSimTime;        ///< seconds

  CommandLine cmd;
  cmd.AddValue ("mobility", "1=trace;2=twoNodes", m_mobility);
  cmd.AddValue ("nodes", "Number of nodes (i.e. vehicles)", m_nNodes);
  cmd.AddValue ("totaltime", "Simulation end time", m_TotalSimTime);

  //  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
    cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.Parse (argc, argv);
  //Convert to time object
  Time interPacketInterval = Seconds (interval);

  std::cout << std::setw (13) << "Time" <<
    std::setw (12) << "Tput (Mb/s)" <<
    std::setw (10) << "Received " <<
    std::setw (12) << "Signal (dBm)" <<
    std::setw (12) << "Noi+Inf(dBm)" <<
    std::setw (9) << "SNR (dB)" <<
    std::endl;


  NS_LOG_INFO ("Create nodes.");


  NodeContainer c;
  c.Create (m_nNodes);




  // The below set of helpers will help us to put together the wifi NICs we want
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
    // Propagation loss models are additive.
  // modelo de perdas
  m_lossModelName = "ns3::FriisPropagationLossModel";
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (5.9e9));
  wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");

  // the channel

  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  wifiPhy.SetChannel (channel);

  // ns-3 supports generate a pcap trace
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
  if (verbose)
    {
      wifi80211p.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
    }

  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode),
                                      "ControlMode",StringValue (phyMode));
  NetDeviceContainer devices = wifi80211p.Install (wifiPhy, wifi80211pMac, c);


  // Tracing
  wifiPhy.EnablePcap ("wave-simple-80211p", devices);

  if (m_mobility == 1)
    {
      // Create Ns2MobilityHelper with the specified trace log file as parameter
      Ns2MobilityHelper ns2 = Ns2MobilityHelper (m_traceFile);
      ns2.Install (); // configure movements for each node, while reading trace file
      // initially assume all nodes packet not moving
      WaveBsmHelper::GetNodesMoving ().resize (m_nNodes, 0);
    } else if (m_mobility == 2)
      {
          MobilityHelper mobility;
          Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
          positionAlloc->Add (Vector (0.0, 0.0, 0.0));
          positionAlloc->Add (Vector (5.0, 0.0, 0.0));
          mobility.SetPositionAllocator (positionAlloc);
          mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
          mobility.Install (c);
        }



  /*if (c.Get(3))
  //        {
              MobilityHelper mobility;
              Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
              positionAlloc->Add (Vector (0.0, 0.0, 0.0));
            //  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
              mobility.SetPositionAllocator (positionAlloc);
              mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
              mobility.Install (c.Get(3));
          }
*/
  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  uint16_t port = 4000;
  for (int conta_nodos=0;conta_nodos<=2;conta_nodos++){

// take into account all the nodes except the one that transmits the video
    if (conta_nodos != 1) {
      Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (conta_nodos), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
      recvSink->Bind (local);
      recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

    }else{
    Ptr<Socket> source = Socket::CreateSocket (c.Get (conta_nodos), tid);
    InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
    source->SetAllowBroadcast (true);
    source->Connect (remote);


    }
}


// Create one EvalvidClient application -server

EvalvidServerHelper server (port);
server.SetAttribute ("SenderTraceFilename", StringValue("st_highway_cif.st"));
server.SetAttribute ("SenderDumpFilename", StringValue("sd_a01"));
server.SetAttribute ("PacketPayload",UintegerValue(packetSize));
ApplicationContainer apps = server.Install (c.Get(1));

apps.Start (Seconds (1.0));
apps.Stop (Seconds (m_TotalSimTime+1.0));

// Create one EvalvidClient application -client

EvalvidClientHelper client (i.GetAddress (1),port);
client.SetAttribute ("ReceiverDumpFilename", StringValue("rd_a01"));
apps = client.Install (c.Get (0));
apps.Start (Seconds (2.0));
apps.Stop (Seconds (m_TotalSimTime));


Config::ConnectWithoutContext ("/NodeList/0/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback (&MonitorSniffRx));

  //
  // Create one udpServer applications on node one.
  // nó 2	<node id="n2" x="+250.0" y="+400.0" /> vant que gerou imagens





  //	nó 3 <node id="n3" x="+300.0" y="+300.0" />

  // nó 1	<node id="n1" x="+150.0" y="+350.0" />


  // nó 0 corresponde a base <node id="n0" x="+0.0" y="+0.0" /> não apresenta mobilidade
  //aqui corresponde ao nó 3



/*  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, packetSize, interPacketInterval);
*/

  AnimationInterface anim("animationwave80211p_evalvid.xml");
  //
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
//  LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);
  Simulator::Run ();

  double throughput = 0;
  uint64_t totalPacketsThrough = 0;

    //  totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (1))->GetReceived ();
      totalPacketsThrough = pktCount_n;
      throughput = totalPacketsThrough * packetSize * 8 / (m_TotalSimTime * 1000000.0); //Mbit/s

  std::cout << std::setprecision (2) << std::fixed <<
        std::setw (12) << throughput <<
        std::setw (8) << totalPacketsThrough;
      if (totalPacketsThrough > 0)
        {
          std::cout << std::setw (12) << g_signalDbmAvg <<
            std::setw (12) << g_noiseDbmAvg <<
            std::setw (12) << (g_signalDbmAvg - g_noiseDbmAvg) <<
            std::endl;
        }
      else
        {
          std::cout << std::setw (12) << "N/A" <<
            std::setw (12) << "N/A" <<
            std::setw (12) << "N/A" <<
            std::endl;
        }

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");


  return 0;
}
