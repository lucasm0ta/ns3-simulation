#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

/*
    Baseado em https://www.nsnam.org/docs/tutorial/html/building-topologies.html#building-a-wireless-network-topology
*/

///////////////////////////////////////////////////////////////////
//                             TOPOLOGIA
//
//     WIFI  10.1.3.0                                     WIFI  10.1.4.0
//            AP                                      AP
//  *         *                                       *         *
//  |  . . .  |       p2p                  p2p        | . . . . |
// n10        n0----------------n0--------------------n0        n10
//                              |
//                              |       n10   servidor
//                              |  . . . |       |
//                              ==================
//                                 LAN  10.1.2.0
///////////////////////////////////////////////////////////////////

    
NS_LOG_COMPONENT_DEFINE("FirstScriptExample");
int main(int argc, char *argv[])
{
    bool verbose = true;
    uint32_t nCsma = 10;
    uint32_t nWifi = 10;
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
        LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    //helper para os BUS
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    
    //Nós que conectarão as redes entre si
    /*NodeContainer connectionNodes;
    connectionNodes.Create(3);*/
    NodeContainer w1_to_busNodes, w2_to_busNodes;
    w1_to_busNodes.Create (2);
    w2_to_busNodes.Add(w1_to_busNodes.Get(0));
    w2_to_busNodes.Create (1);
  
    PointToPointHelper w_to_bus;
    w_to_bus.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    w_to_bus.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer w1_to_busDevices, w2_to_busDevices;
    w1_to_busDevices = w_to_bus.Install (w1_to_busNodes);
    w2_to_busDevices = w_to_bus.Install (w2_to_busNodes);
    
    //Nodes que serão parte do BUS da LAN
    NodeContainer csmaNodes;
    csmaNodes.Add(w1_to_busNodes.Get(0));
    csmaNodes.Create(nCsma);
    
    //Seta os devices de cada container
    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);
    
    
    /////////////////////////////////////////////
    //                WIFI 1
    /////////////////////////////////////////////
    
    NodeContainer wifiStaNodes1, wifiStaNodes2;
    wifiStaNodes1.Create(nWifi);
    wifiStaNodes2.Create(nWifi);
    
    NodeContainer wifiApNode1 = w1_to_busNodes.Get(1);
    NodeContainer wifiApNode2 = w2_to_busNodes.Get(1);
    
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
    mac1.SetType("ns3::StaWifiMac",
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
    
    //Nós que são pontos de acesso
    NetDeviceContainer apDevices1, apDevices2;
    apDevices1 = wifi.Install(phy1, mac1, wifiApNode1);
    apDevices2 = wifi.Install(phy2, mac2, wifiApNode2);
    
    MobilityHelper mobility1, mobility2;

    //Nós wireless podem se mover.
    mobility1.SetPositionAllocator("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue(0.0),
                                   "MinY", DoubleValue(0.0),
                                   "DeltaX", DoubleValue(5.0),
                                   "DeltaY", DoubleValue(10.0),
                                   "GridWidth", UintegerValue(3),
                                   "LayoutType", StringValue("RowFirst"));
    mobility2.SetPositionAllocator("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue(30.0),
                                   "MinY", DoubleValue(0.0),
                                   "DeltaX", DoubleValue(5.0),
                                   "DeltaY", DoubleValue(10.0),
                                   "GridWidth", UintegerValue(3),
                                   "LayoutType", StringValue("RowFirst"));
    
    //Faz os nós andarem aleatoriamente 
    mobility1.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                               "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility2.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                               "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
                               
    //instala capacidade de se mover                       
    mobility1.Install(wifiStaNodes1);                       
    mobility2.Install(wifiStaNodes2);
    
    //Ponto de acesso não se move
    mobility1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility2.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility1.Install(wifiApNode1);
    mobility2.Install(wifiApNode2);
    
    /////////////////////////////////////////////
    //         ORGANIZAR PROTOCOLOS 
    /////////////////////////////////////////////
    InternetStackHelper stack;
    stack.Install(csmaNodes);
    stack.Install(wifiApNode1);
    stack.Install(wifiApNode2);
    stack.Install(wifiStaNodes1);
    stack.Install(wifiStaNodes2);
    
    Ipv4AddressHelper address;

    //Bus que conecta os pontos de acesso e n0 da LAN
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer w1_to_busInterfaces;
    w1_to_busInterfaces = address.Assign (w1_to_busDevices);
    
    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer w2_to_busInterfaces;
    w2_to_busInterfaces = address.Assign (w2_to_busDevices);
    
    //Bus que conecta os nós da LAN
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);
    
    //coexão entre elemenstos da WIFI 1
    address.SetBase("10.1.4.0", "255.255.255.0");
    address.Assign(staDevices1);
    address.Assign(apDevices1);
    
    //coexão entre elemenstos da WIFI 2
    address.SetBase("10.1.5.0", "255.255.255.0");
    address.Assign(staDevices2);
    address.Assign(apDevices2);
    
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(csmaNodes.Get(nCsma));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(30.0));

    UdpEchoClientHelper echoClient(csmaInterfaces.GetAddress(nCsma), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    //Torna os pontos de acesso como echoClients
    ApplicationContainer clientApps1 = echoClient.Install(wifiStaNodes1.Get(nWifi - 1));
    clientApps1.Start(Seconds(0.0));
    clientApps1.Stop(Seconds(30.0));
        
    ApplicationContainer clientApps2 = echoClient.Install(wifiStaNodes2.Get(nWifi - 1));
    clientApps2.Start(Seconds(0.0));
    clientApps2.Stop(Seconds(30.0));
    
    //Popula tabela de roteamento
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    Simulator::Stop(Seconds(30.0));
    
    w_to_bus.EnablePcapAll ("third");
    csma.EnablePcap("network_sim", csmaDevices.Get(0), true);
    phy1.EnablePcap("network_sim", apDevices1.Get(0));
    phy2.EnablePcap("network_sim", apDevices2.Get(0));
    
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
