 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/ipv4-global-routing-helper.h"
 #include "ns3/netanim-module.h"
 #include "ns3/v4ping-helper.h"
  
 using namespace ns3;
  
 //NS_LOG_COMPONENT_DEFINE ("UdpServer");
  
 int 
 main (int argc, char *argv[])
 {

    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
    //LogComponentEnable ("UdpSocketImpl", LOG_LEVEL_ALL);
    LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(1000)); //bytes per packet
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("1Kb/s"));
    uint32_t N = 11; //number of nodes in the star
    uint32_t K = 1; //number of attackers
  
    CommandLine cmd (__FILE__);
    cmd.AddValue("nNodes", "Number of nodes to place in the star", N);
    cmd.Parse(argc, argv);

    NodeContainer server;
    NodeContainer clients;
    NodeContainer malicious;
    server.Create(1);
    clients.Create(N-1);
    malicious.Create(K);
    NodeContainer nodes = NodeContainer(server, clients, malicious);

    InternetStackHelper internet;
    internet.Install(nodes);

    std::vector<NodeContainer> nodeAdjacencyList(N-1);
    for(uint32_t i = 0; i < nodeAdjacencyList.size(); ++i) {
        nodeAdjacencyList[i] = NodeContainer(server, clients.Get(i));
    } 

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Kbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    std::vector<NetDeviceContainer> deviceAdjacencyList(N-1);
    for(uint32_t i = 0; i < deviceAdjacencyList.size(); ++i) {
        deviceAdjacencyList[i] = p2p.Install(nodeAdjacencyList[i]);
    }
  
    Ipv4AddressHelper ipv4;
    std::vector<Ipv4InterfaceContainer> interfaceAdjacencyList(N-1);
    for(uint32_t i = 0; i < interfaceAdjacencyList.size(); ++i) {
        std::ostringstream subnet;
        subnet<<"10.1."<<i+1<<".0";
        ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
        interfaceAdjacencyList[i] = ipv4.Assign(deviceAdjacencyList[i]);
    }
  
    // Create a packet sink on the star "hub" to receive packets
    uint16_t port = 50000;
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install(server);
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(100.0));

    OnOffHelper clientHelper("ns3::UdpSocketFactory", Address());
    clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApps;
    for(uint32_t i = 0; i < clients.GetN(); ++i) {
           AddressValue remoteAddress
             (InetSocketAddress(interfaceAdjacencyList[i].GetAddress(0), port));
           clientHelper.SetAttribute("Remote", remoteAddress);
           clientApps.Add(clientHelper.Install(clients.Get(i)));
    }
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(120.0));

    V4PingHelper ping1 (interfaceAdjacencyList[0].GetAddress(0));
    ping1.SetAttribute ("Verbose", BooleanValue (false));
    ping1.SetAttribute ("Interval", TimeValue (Seconds(1.0)));
    ping1.SetAttribute ("Size", UintegerValue (16));
    //ping1.SetAttribute ("DataRate", UintegerValue(16));

    ApplicationContainer appPingInternet1;
    for(uint32_t i = 0; i < nodeAdjacencyList.size(); ++i){
        appPingInternet1.Add(ping1.Install(clients.Get(i)));
    }
    appPingInternet1.Start (Seconds (3.0));
    appPingInternet1.Stop (Seconds (120.0));

    std::vector<NodeContainer> nodeAdjacencyList2(K-1);
    for(uint32_t i = 0; i < nodeAdjacencyList2.size(); ++i) {
        nodeAdjacencyList2[i] = NodeContainer(server, malicious.Get(i));
    } 

    PointToPointHelper p2pM;
    p2pM.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pM.SetChannelAttribute("Delay", StringValue("1ms"));
    std::vector<NetDeviceContainer> deviceAdjacencyList2(K-1);
    for(uint32_t i = 0; i < deviceAdjacencyList2.size(); ++i) {
        deviceAdjacencyList2[i] = p2pM.Install(nodeAdjacencyList2[i]);
    }
  
    Ipv4AddressHelper ipv4M;
    std::vector<Ipv4InterfaceContainer> interfaceAdjacencyList2(K-1);
    for(uint32_t i = 0; i < interfaceAdjacencyList2.size(); ++i) {
        std::ostringstream subnet2;
        subnet2<<"10.2."<<i+1<<".0";
        ipv4M.SetBase (subnet2.str ().c_str (), "255.255.255.128");
        interfaceAdjacencyList2[i] = ipv4M.Assign(deviceAdjacencyList2[i]);
    }

    UdpClientHelper client2 (interfaceAdjacencyList[0].GetAddress(0), port);
    uint32_t MaxPacketSize = 10000;
    Time interPacketInterval = Seconds(1.0);
    //uint32_t maxPacketCount = 320;
    //client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client2.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));

    ApplicationContainer clientApps2;
    for(uint32_t i = 0; i < K; ++i) {
           clientApps2.Add(client2.Install(malicious.Get(i)));
           //clientApps2.Add(client2.Install(clients.Get(i+2)));
    }
    clientApps2.Start(Seconds(15.0));
    clientApps2.Stop(Seconds(100.0));

    /*ApplicationContainer clientApps2;
    for(uint32_t i = 0; i < K; i++) {
        UdpClientHelper client2 (interfaceAdjacencyList2[i].GetAddress(0), port);
        //client2.SetAttribute("Interval", TimeValue (Seconds(0.5)));
        //client2.SetAttribute("PacketSize", UintegerValue (10000));
        clientApps2.Add(client2.Install(malicious.Get(i)));
    }
    clientApps2.Start(Seconds(15.0));
    clientApps2.Stop(Seconds(100.0));*/

    /*BulkSendHelper bulkSend();
    for(uint32_t i = 0; i < K; ++i) {
        bulkSend("ns3::UdpSocketFactory",
         InetSocketAddress(interfaceAdjacencyList2[i].GetAddress(2), port));
    }

    ApplicationContainer bulk;
    for(uint32_t i = 0; i < K; ++i) {
        bulk.Add(bulkSend.Install(malicious.Get(i)));
    }
    bulk.Start(Seconds(15.0));
    bulk.Stop(Seconds(100.0));*/
    
    /*V4PingHelper ping2 (interfaceAdjacencyList[0].GetAddress(0));
    ping1.SetAttribute ("Verbose", BooleanValue (false));
    ping1.SetAttribute ("Interval", StringValue("1ms"));
    ping1.SetAttribute ("Size", UintegerValue (1000));*/

    /*ApplicationContainer appPingInternet2;
    for(uint32_t i = 0; i < nodeAdjacencyList2.size(); i++){
        appPingInternet2 = ping2.Install(nodeAdjacencyList2[i].Get(1));
    }
    appPingInternet2.Start (Seconds (30.0));
    appPingInternet2.Stop (Seconds (100.0));*/

    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    AnimationInterface animation("star-topology.xml");
    const double centerX = 0.0;
    const double centerY = 0.0;
    const double radius = 50.0;
    const double angle = 2.0 * 3.1415926 / clients.GetN ();
    for (uint32_t i = 0; i < clients.GetN (); ++i) {
        double x = centerX + radius * std::cos (angle * i);
        double y = centerY + radius * std::sin (angle * i);
        animation.SetConstantPosition (clients.Get (i), x, y);
    }
     
    animation.SetConstantPosition (server.Get (0), centerX, centerY);

    //AsciiTraceHelper ascii;
    //p2p.EnableAsciiAll(ascii.CreateFileStream ("star-ping.tr"));
    //p2pM.EnableAsciiAll(ascii.CreateFileStream ("star-ping-M.tr"));


    p2p.EnablePcapAll ("teste2");

    //NS_LOG_INFO ("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    //NS_LOG_INFO ("Done.");
  
    return 0;
 }