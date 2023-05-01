 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/ipv4-global-routing-helper.h"
 #include "ns3/netanim-module.h"
  
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
    uint32_t N = 10; //number of nodes in the star
  
    CommandLine cmd (__FILE__);
    cmd.AddValue("nNodes", "Number of nodes to place in the star", N);
    cmd.Parse(argc, argv);

    NodeContainer server;
    NodeContainer clients;
    server.Create(1);
    clients.Create(N-1);
    NodeContainer nodes = NodeContainer(server, clients);

    InternetStackHelper internet;
    internet.Install(nodes);

    std::vector<NodeContainer> nodeAdjacencyList(N-1);
    for(uint32_t i = 0; i < nodeAdjacencyList.size(); ++i) {
        nodeAdjacencyList[i] = NodeContainer(server, clients.Get (i));
    } 

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Kbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1000ms"));
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
    uint16_t port = 1024;
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install(server);
    sinkApp.Start(Seconds(2.0));
    sinkApp.Stop(Seconds(30.0));

    OnOffHelper clientHelper("ns3::UdpSocketFactory", Address());
    clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  
    //normally wouldn't need a loop here but the server IP address is different
    //on each p2p subnet
    ApplicationContainer clientApps;
    for(uint32_t i = 0; i < clients.GetN(); ++i) {
           AddressValue remoteAddress
             (InetSocketAddress(interfaceAdjacencyList[i].GetAddress(0), port));
           clientHelper.SetAttribute("Remote", remoteAddress);
           clientApps.Add(clientHelper.Install(clients.Get(i)));
    }
    clientApps.Start(Seconds(3.0));
    clientApps.Stop(Seconds(30.0));

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
  
    p2p.EnablePcapAll ("teste2");

    //NS_LOG_INFO ("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    //NS_LOG_INFO ("Done.");
  
    return 0;
 }