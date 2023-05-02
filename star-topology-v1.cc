 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/ipv4-global-routing-helper.h"
 #include "ns3/netanim-module.h"
 #include "ns3/v4ping-helper.h"
 #include "ns3/mobility-module.h"
 #include "ns3/random-walk-2d-mobility-model.h"
  
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
  uint32_t N = 101; //number of nodes in the star
  
  CommandLine cmd (__FILE__);
  cmd.AddValue("nNodes", "Number of nodes to place in the star", N);
  cmd.Parse(argc, argv);

  NodeContainer server;
  NodeContainer clients;
  server.Create(1);
  clients.Create(N-1);
  NodeContainer nodes = NodeContainer(server, clients);
     
  //Establish internet stack between all nodes in the simulation
  InternetStackHelper internet;
  internet.Install(nodes);
  
  /*Necessary to establish p2p connections, as they can only be installed 
  in NetDeviceContainers, which can only store NodeContainers*/
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
  
  /*Ipv4 addresses have to be set in Ipv4InterfaceContainers, which store NetDeviceContainers*/ 
  Ipv4AddressHelper ipv4;
  std::vector<Ipv4InterfaceContainer> interfaceAdjacencyList(N-1);
  for(uint32_t i = 0; i < interfaceAdjacencyList.size(); ++i) {
      std::ostringstream subnet;
      subnet<<"10.1."<<i+1<<".0";
      ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      interfaceAdjacencyList[i] = ipv4.Assign(deviceAdjacencyList[i]);
  }
     
  // enable checksums to avoid error in ping reply (checksum verification goes wrong even if disabled) 
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true)); 
  
  // Create a packet sink on the star "hub" to receive packets
  uint16_t port = 50000;
  Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
  PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install(server);
  sinkApp.Start(Seconds(1.0));
  sinkApp.Stop(Seconds(30.0));

  /*OnOffHelper clientHelper("ns3::UdpSocketFactory", Address());
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
  clientApps.Stop(Seconds(120.0));*/
 
  // Initialise V4PinHelper with target address as a parameter
  V4PingHelper ping (interfaceAdjacencyList[0].GetAddress(0));
  ping.SetAttribute ("Verbose", BooleanValue (false));
  ping.SetAttribute ("Interval", TimeValue (Seconds(1.0)));
  ping.SetAttribute ("Size", UintegerValue (16));

  ApplicationContainer appPing;
  for(uint32_t i = 0; i < nodeAdjacencyList.size(); ++i){
      appPing.Add(ping.Install(clients.Get(i)));
  }
  appPing.Start(Seconds (3.0));
  appPing.Stop(Seconds (30.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  AnimationInterface animation("star-topology.xml");

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (5.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  //If nodes have to move, this is an alternative  
  //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             //"Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  //mobility.Install (clients);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  const double centerX = 30.0;
  const double centerY = 30.0;
  const double radius = 20.0;
  const double angle = 2.0 * 3.1415926 / clients.GetN ();
  for (uint32_t i = 0; i < clients.GetN (); ++i) {
    double x = centerX + radius * std::cos (angle * i);
    double y = centerY + radius * std::sin (angle * i);
    animation.SetConstantPosition (clients.Get (i), x, y);
  }
     
  animation.SetConstantPosition (server.Get (0), centerX, centerY);

  p2p.EnablePcapAll ("teste2");

  Simulator::Run();
  Simulator::Destroy();
  
  return 0;
}