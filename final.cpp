#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpPerformanceTest");

void PrintTcpThroughput(Ptr<FlowMonitor> flowMonitor, FlowMonitorHelper &monitorHelper) {
    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMonitor->GetFlowStats();
    for (auto &flow : flowStats) {
        double throughputKbps = (flow.second.rxBytes * 8.0 /
                                 (flow.second.timeLastRxPacket.GetSeconds() -
                                  flow.second.timeFirstTxPacket.GetSeconds())) /
                                1024;
        std::cout << "Flow ID " << flow.first << ": Throughput = " << throughputKbps << " Kbps" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    std::string linkDelay = "10ms";

    CommandLine cmd;
    cmd.AddValue("linkDelay", "Point-to-Point link delay", linkDelay);
    cmd.Parse(argc, argv);

    std::cout << "Starting TCP performance test with link delay = " << linkDelay << std::endl;

    // Create two nodes
    NodeContainer networkNodes;
    networkNodes.Create(2);

    // Configure Point-to-Point link attributes
    PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2pHelper.SetChannelAttribute("Delay", StringValue(linkDelay));

    NetDeviceContainer p2pDevices = p2pHelper.Install(networkNodes);

    // Install Internet Protocol stack
    InternetStackHelper internetStack;
    internetStack.Install(networkNodes);

    // Assign IP addresses to the devices
    Ipv4AddressHelper ipv4AddrHelper;
    ipv4AddrHelper.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipInterfaces = ipv4AddrHelper.Assign(p2pDevices);

    std::cout << "Setting up TCP server and client applications..." << std::endl;

    // Define TCP Server and Client
    uint16_t serverPort = 5000;
    Address serverAddress(InetSocketAddress(ipInterfaces.GetAddress(1), serverPort));
    PacketSinkHelper tcpServerHelper("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApp = tcpServerHelper.Install(networkNodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    OnOffHelper tcpClientHelper("ns3::TcpSocketFactory", serverAddress);
    tcpClientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpClientHelper.SetAttribute("DataRate", StringValue("5Mbps"));
    tcpClientHelper.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp = tcpClientHelper.Install(networkNodes.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    std::cout << "Launching simulation..." << std::endl;

    // Enable Flow Monitoring
    FlowMonitorHelper flowMonitorHelper;
    Ptr<FlowMonitor> monitorInstance = flowMonitorHelper.InstallAll();

    // Run the simulation
    Simulator::Stop(Seconds(12.0));
    Simulator::Run();

    std::cout << "Simulation finished. Calculating TCP throughput..." << std::endl;

    // Print throughput statistics
    PrintTcpThroughput(monitorInstance, flowMonitorHelper);

    // Cleanup simulation
    Simulator::Destroy();
    std::cout << "TCP performance test completed!" << std::endl;

    return 0;
}
