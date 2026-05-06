#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>

using namespace ns3;

// Clase Wormhole
class WormholeTunnel {
public:
    WormholeTunnel(Ptr<Node> n1, Ptr<Node> n2) {
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        p2p.SetChannelAttribute("Delay", StringValue("1ms"));

        NetDeviceContainer devs = p2p.Install(n1, n2);

        InternetStackHelper stack;
        stack.Install(n1);
        stack.Install(n2);

        Ipv4AddressHelper addr;
        addr.SetBase("192.168.1.0", "255.255.255.0");
        addr.Assign(devs);
    }
};

int main(int argc, char *argv[]) {

    uint32_t run = 1;
    double simulationTime = 100.0;

    CommandLine cmd;
    cmd.AddValue("run", "Run number", run);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(12345);
    RngSeedManager::SetRun(run);

    NodeContainer nodes;
    nodes.Create(30);

    // Movilidad
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=80.0]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=80.0]"));

    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
        "Pause", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));

    mobility.Install(nodes);

    // WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // AODV
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // NODOS MALICIOSOS
    Ptr<Node> attacker1 = nodes.Get(5);
    Ptr<Node> attacker2 = nodes.Get(20);

    // CREAR TÚNEL WORMHOLE
    WormholeTunnel wormhole(attacker1, attacker2);

    // Tráfico
    uint16_t port = 9;

    for (int i = 0; i < 5; i++) {
        OnOffHelper onoff("ns3::UdpSocketFactory",
            InetSocketAddress(interfaces.GetAddress(i+1), port));

        onoff.SetConstantRate(DataRate("256kbps"));
        onoff.SetAttribute("PacketSize", UintegerValue(512));

        ApplicationContainer app = onoff.Install(nodes.Get(0));
        app.Start(Seconds(1.0));
        app.Stop(Seconds(simulationTime));

        PacketSinkHelper sink("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port));

        ApplicationContainer sinkApp = sink.Install(nodes.Get(i+1));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simulationTime));
    }

    // FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    double txPackets = 0, rxPackets = 0, delaySum = 0, rxBytes = 0;

    auto stats = monitor->GetFlowStats();

    for (auto &flow : stats) {
        txPackets += flow.second.txPackets;
        rxPackets += flow.second.rxPackets;
        delaySum += flow.second.delaySum.GetSeconds();
        rxBytes += flow.second.rxBytes;
    }

    double pdr = (rxPackets / txPackets) * 100;
    double avgDelay = delaySum / rxPackets;
    double throughput = (rxBytes * 8.0) / (simulationTime * 1000);

    std::ofstream out("results_wormhole.csv", std::ios::app);
    out << run << "," << pdr << "," << avgDelay << "," << throughput << std::endl;
    out.close();

    Simulator::Destroy();
    return 0;
}
