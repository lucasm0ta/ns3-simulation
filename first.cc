#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

/*
    Baseado em https://www.nsnam.org/docs/tutorial/html/building-topologies.html#building-a-wireless-network-topology
*/

///////////////////////////////////////////////////////////////////
//                             TOPOLOGIA
//
//     WIFI 2  10.1.3.0                                     WIFI 1 10.1.1.0
//            AP                                     AP
//  *         *                                      *         *
//  |  . . .  |                                      | . . . . |
// n10        n0-------------[switch]---------------n0        n10
//                              |
//                              |
//                     ==================   [bridge]  10.1.1.0
//                     |  . . . |       |
//                     n0      n10   servidor
///////////////////////////////////////////////////////////////////

    
NS_LOG_COMPONENT_DEFINE("FirstScriptExample");
int main(int argc, char *argv[])
{
    bool verbose = true;
    int nCsma = 10;
    int nWifi = 10;
    bool tracing = false;
    
    CommandLine cmd;
    cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

    cmd.Parse (argc,argv);
    
    if(nWifi > 250 || nCsma > 250) {
        std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
        return 1;
    }
    if (verbose){
        std::cout <<"Verbose"<<std::endl;
        LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    
    Ptr<Node> switchNode = CreateObject<Node> ();
    
    Ptr<Node> bridgeCsmaNode = CreateObject<Node> ();
    
    NodeContainer routerNodes;
    routerNodes.Add(switchNode);
    /////////////////////////////////////////////
    //                LAN
    /////////////////////////////////////////////
    
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    //Nodes que serão parte da LAN
    NodeContainer csmaNodes;
    csmaNodes.Create(nCsma);
    csmaNodes.Add(switchNode);
    // std::cout <<"Csma Nodes: "<<csmaNodes.size() <<std::endl;
    
    NetDeviceContainer csmaDevices;
    NetDeviceContainer bridgeCsmaDevices;
    for (int i = 0; i < nCsma + 1; i++){
        //Instala um canal csma entre o iésimo device da LAN e o bridge
        NetDeviceContainer link = csma.Install(NodeContainer(csmaNodes.Get(i), bridgeCsmaNode));
        if (i<nCsma) routerNodes.Add(csmaNodes.Get(i));
        csmaDevices.Add(link.Get (0));
        bridgeCsmaDevices.Add(link.Get (1));
    }
    
    BridgeHelper bridge;
    bridge.Install(bridgeCsmaNode, bridgeCsmaDevices);
    
    /////////////////////////////////////////////
    //                WIFI 1
    /////////////////////////////////////////////
    
    NodeContainer wifiStaNodes1, wifiStaNodes2;
    wifiStaNodes1.Create(nWifi);
    wifiStaNodes2.Create(nWifi);
    
    for(int i = 0; i < nWifi; i++) {
        routerNodes.Add(wifiStaNodes1.Get(i));
        routerNodes.Add(wifiStaNodes2.Get(i));
    }
    
    NodeContainer wifiApNode1;
    NodeContainer wifiApNode2;
    wifiApNode1.Create(1);
    wifiApNode2.Create(1);
    
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    
    NetDeviceContainer w1_to_switch, w2_to_switch;
    w1_to_switch = pointToPoint.Install(NodeContainer(wifiApNode1, switchNode));
    w2_to_switch = pointToPoint.Install(NodeContainer(wifiApNode2, switchNode));
    
    routerNodes.Add(wifiApNode2.Get(0));
    routerNodes.Add(wifiApNode1.Get(0));
    
    //WTF
    YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy1 = YansWifiPhyHelper::Default();
    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default();
    
    phy1.SetChannel(channel1.Create());
    phy2.SetChannel(channel2.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac1;
    Ssid ssid1 = Ssid("wifi1");
    mac1.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid1),
                 "ActiveProbing", BooleanValue(false));
    WifiMacHelper mac2;
    Ssid ssid2 = Ssid("wifi2");
    mac2.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid2),
                 "ActiveProbing", BooleanValue(false));
    
    //Nós que acessam a rede wifi
    NetDeviceContainer staDevices1, staDevices2;
    staDevices1 = wifi.Install(phy1, mac1, wifiStaNodes1);
    staDevices2 = wifi.Install(phy2, mac2, wifiStaNodes2);
    
    //
    mac1.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid1));
    mac2.SetType("ns3::ApWifiMac",
                 "Ssid", SsidValue(ssid2));
    
    //Nós que são pontos de acesso com as respectivas configurações
    NetDeviceContainer apDevices1, apDevices2;
    apDevices1 = wifi.Install(phy1, mac1, wifiApNode1.Get(0));
    apDevices2 = wifi.Install(phy2, mac2, wifiApNode2.Get(0));
    
    //Posiciona os nós das WIFI
    MobilityHelper mobility1, mobility2;
    Ptr<ListPositionAllocator> posAlloc1 =  CreateObject<ListPositionAllocator>();
    posAlloc1->Add (Vector (-50.0, 45.0, 0.0));
    posAlloc1->Add (Vector (-60.0, 45.0, 0.0));
    posAlloc1->Add (Vector (-70.0, 45.0, 0.0));
    posAlloc1->Add (Vector (-80.0, 45.0, 0.0));
    posAlloc1->Add (Vector (-80.0, 35.0, 0.0));
    posAlloc1->Add (Vector (-80.0, 25.0, 0.0));
    posAlloc1->Add (Vector (-80.0, 15.0, 0.0));
    posAlloc1->Add (Vector (-70.0, 15.0, 0.0));
    posAlloc1->Add (Vector (-60.0, 15.0, 0.0));
    posAlloc1->Add (Vector (-50.0, 15.0, 0.0));
    posAlloc1->Add (Vector (-50.0, 30.0, 0.0));
    mobility1.SetPositionAllocator (posAlloc1);

    Ptr<ListPositionAllocator> posAlloc2 =  CreateObject<ListPositionAllocator>();
    posAlloc2->Add (Vector (-50.0, -45.0, 0.0));
    posAlloc2->Add (Vector (-60.0, -45.0, 0.0));
    posAlloc2->Add (Vector (-70.0, -45.0, 0.0));
    posAlloc2->Add (Vector (-80.0, -45.0, 0.0));
    posAlloc2->Add (Vector (-80.0, -35.0, 0.0));
    posAlloc2->Add (Vector (-80.0, -25.0, 0.0));
    posAlloc2->Add (Vector (-80.0, -15.0, 0.0));
    posAlloc2->Add (Vector (-70.0, -15.0, 0.0));
    posAlloc2->Add (Vector (-60.0, -15.0, 0.0));
    posAlloc2->Add (Vector (-50.0, -15.0, 0.0));
    posAlloc2->Add (Vector (-50.0, -30.0, 0.0));
    mobility2.SetPositionAllocator (posAlloc2);

    mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    //Outros nos da WIFI
    mobility1.Install(wifiStaNodes1);
    mobility2.Install(wifiStaNodes2);
    
    //Ponto de acesso
    mobility1.Install(wifiApNode1);
    mobility2.Install(wifiApNode2);
    
    /////////////////////////////////////////////
    //         ORGANIZAR PROTOCOLOS 
    /////////////////////////////////////////////
    InternetStackHelper internet;
    internet.Install (routerNodes);
    
    Ipv4AddressHelper address;
    
    //Bus que conecta os nós da LAN
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);
    
    //coexão entre elemenstos da WIFI 1
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer wifi1Interfaces;
    wifi1Interfaces = address.Assign(staDevices1);
    address.Assign(apDevices1);
    address.Assign(w1_to_switch);
    
    //conexão entre elemenstos da WIFI 2
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer wifi2Interfaces;
    wifi2Interfaces = address.Assign(staDevices2);
    address.Assign(apDevices2);
    address.Assign(w2_to_switch);
    
    //Popula tabela de roteamento
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    //Estrutura Cliente-Servidor
    //
    // Create an OnOff application to send UDP datagrams from node zero to node 1.
    //
    NS_LOG_INFO ("Create Applications.");
    uint16_t port = 9;   // Discard port (RFC 863)
    
    OnOffHelper onoff ("ns3::UdpSocketFactory", 
                        Address(InetSocketAddress(Ipv4Address ("10.1.1.4"), port)));
    onoff.SetConstantRate (DataRate ("500kb/s"));
    
    ApplicationContainer app = onoff.Install (wifiStaNodes2.Get(5));
    // Start the application
    app.Start (Seconds (0.0));
    app.Stop (Seconds (10.0));
    /*
    // Create an optional packet sink to receive these packets
    PacketSinkHelper sink ("ns3::UdpSocketFactory",
    Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
    ApplicationContainer sink1 = sink.Install (csmaNodes.Get(3));
    sink1.Start (Seconds (0.0));
    sink1.Stop (Seconds (10.0));*/

    Simulator::Stop(Seconds(30.0));
    
    csma.EnablePcap("network_sim", csmaDevices.Get(0), true);
    phy1.EnablePcap("network_sim", apDevices1.Get(0));
    phy2.EnablePcap("network_sim", apDevices2.Get(0));
    
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
