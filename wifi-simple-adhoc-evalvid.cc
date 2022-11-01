/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 *
 */

// This script configures two nodes on an 802.11n physical layer, with
// 802.11n NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
// for mobility and Evalvid
//  ./waf --run "scratch/wifi-simple-adhoc-evalvid --mobility=1 --nodes=3 --totaltime=90 --packetSize=1472 --interval=0.1 --verbose=false" 2>&1 | tee testeEvalvid_80211n.txt

#include "ns3/vector.h"
#include "ns3/socket.h"
#include "ns3/position-allocator.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include "ns3/netanim-module.h" // simulation lib

// for the video
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-helper.h"
#include "ns3/evalvid-client-server-helper.h"
#include "ns3/wave-bsm-helper.h"
#include "ns3/mobility-module.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");

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
  // Enable logging for EvalvidClient and EvalvidServer
  //
  LogComponentEnable ("EvalvidClient", LOG_LEVEL_INFO);
  LogComponentEnable ("EvalvidServer", LOG_LEVEL_INFO);

  std::string phyMode ("HtMcs1");
  std::string m_traceFile= "/home/stelios/mobility.tcl";
  std::string m_lossModelName;

  uint32_t m_mobility; ///< mobility
  uint32_t m_nNodes; ///< number of nodes
  double m_TotalSimTime;        ///< seconds

//  double rss = -80;  // -dBm
  uint32_t packetSize = 1472; // bytes
//  uint32_t numPackets = 1;
  double interval = 1.0; // seconds
  bool verbose = false;

  CommandLine cmd;
  cmd.AddValue ("mobility", "1=trace;2=twoNodes", m_mobility);
  cmd.AddValue ("nodes", "Number of nodes (i.e. vehicles)", m_nNodes);
  cmd.AddValue ("totaltime", "Simulation end time", m_TotalSimTime);

//  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
//  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
//  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);


  NS_LOG_INFO ("Create nodes.");


  NodeContainer c;
  c.Create (m_nNodes);

  // Channel
    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel;
    // Propagation loss models are additive.
    // modelo de perdas
    m_lossModelName = "ns3::FriisPropagationLossModel";
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (2.4e9));
    wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");

    // the channel


    wifiPhy.SetChannel (wifiChannel.Create ());
    wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
    WifiHelper wifi;
    wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);



    // The below set of helpers will help us to put together the wifi NICs we want
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
//  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue ("HtMcs1"));


  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
//  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));


  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  // Tracing
    wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  if (m_mobility == 1)
    {
      // Create Ns2MobilityHelper with the specified trace log file as parameter
      Ns2MobilityHelper ns2 = Ns2MobilityHelper (m_traceFile);
      ns2.Install (); // configure movements for each node, while reading trace file

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
  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  uint16_t port = 4000;
  for (int conta_nodos=0;conta_nodos<=2;conta_nodos++){

  //Variables for spacing in the simulation (variables that depend on external libraries)
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

    AnimationInterface anim("animationwifi80211n_evalvid.xml");

  /* Output what we are doing
  NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent with receiver rss " << rss );

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); */


  NS_LOG_INFO ("Run Simulation.");
  
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  return 0;
}
