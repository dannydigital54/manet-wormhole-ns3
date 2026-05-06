#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WormholeExperiment");

int main ()
{
    uint32_t nNodes = 30;
    double simTime = 60.0;

    NodeContainer nodes;
    nodes.Create (nNodes);

    // WIFI
    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211b);

    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (YansWifiChannelHelper::Default ().Create ());

    WifiMacHelper mac;
    mac.SetType ("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install (phy, mac, nodes);

    // MOVILIDAD
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
        "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=80]"),
        "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=80]"));

    mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
        "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"));

    mobility.Install (nodes);

    // AODV
    AodvHelper aodv;
    InternetStackHelper stack;
    stack.SetRoutingHelper (aodv);
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    auto interfaces = address.Assign (devices);

    // FLOWS
    uint16_t port = 9;
    for (int i = 0; i < 5; i++)
    {
        OnOffHelper onoff ("ns3::UdpSocketFactory",
            InetSocketAddress (interfaces.GetAddress (nNodes - 1 - i), port));

        onoff.SetConstantRate (DataRate ("256kbps"));
        auto app = onoff.Install (nodes.Get (i));
        app.Start (Seconds (1));
        app.Stop (Seconds (simTime));

        PacketSinkHelper sink ("ns3::UdpSocketFactory",
            InetSocketAddress (Ipv4Address::GetAny (), port));

        auto sinkApp = sink.Install (nodes.Get (nNodes - 1 - i));
        sinkApp.Start (Seconds (0));
    }

    // WORMHOLE (ENLACE DIRECTO)
    NodeContainer wormholeNodes;
    wormholeNodes.Add (nodes.Get (0));
    wormholeNodes.Add (nodes.Get (1));

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));

    NetDeviceContainer wormDevices = p2p.Install (wormholeNodes);

    Ipv4AddressHelper wormAddr;
    wormAddr.SetBase ("20.0.0.0", "255.255.255.0");
    wormAddr.Assign (wormDevices);

    // FLOW MONITOR
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();

    monitor->CheckForLostPackets ();

    double totalTx = 0, totalRx = 0, totalDelay = 0;

    for (auto flow : monitor->GetFlowStats ())
    {
        totalTx += flow.second.txPackets;
        totalRx += flow.second.rxPackets;
        totalDelay += flow.second.delaySum.GetSeconds ();
    }

    double pdr = (totalRx / totalTx) * 100;
    double avgDelay = totalDelay / totalRx;

    std::ofstream out ("results/output.csv", std::ios::app);
    out << pdr << "," << avgDelay << std::endl;

    std::cout << "PDR: " << pdr << "%\n";
    std::cout << "Delay: " << avgDelay << " s\n";

    Simulator::Destroy ();
}