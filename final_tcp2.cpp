#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    // Define default simulation parameters
    uint32_t pktSizeBytes = 1024;               // Packet size (bytes)
    double simDuration = 10.0;                  // Total simulation time (seconds)
    std::string linkRate = "5Mbps";             // Data rate of the Point-to-Point link
    std::vector<std::string> delayOptions = {   // Different latencies to experiment with
        "10ms", "50ms", "100ms", "200ms", "500ms"};

    // Allow command-line customization
    CommandLine cmd;
    cmd.AddValue("linkRate", "Data rate for the Point-to-Point link", linkRate);
    cmd.Parse(argc, argv);

    // Base TCP port for applications
    uint16_t startingPort = 9000;

    // Iterate over the specified latencies to conduct multiple simulations
    for (const std::string &delay : delayOptions)
    {
        // Create a pair of nodes
        NodeContainer networkNodes;
        networkNodes.Create(2);

        // Install the Internet protocol stack on both nodes
        InternetStackHelper internetHelper;
        internetHelper.Install(networkNodes);

        // Configure Point-to-Point channel parameters
        PointToPointHelper p2pHelper;
        p2pHelper.SetDeviceAttribute("DataRate", StringValue(linkRate));
        p2pHelper.SetChannelAttribute("Delay", StringValue(delay));

        // Establish the Point-to-Point link
        NetDeviceContainer p2pDevices = p2pHelper.Install(networkNodes);

        // Assign IP addresses to the nodes
        Ipv4AddressHelper ipHelper;
        ipHelper.SetBase("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer ipInterfaces = ipHelper.Assign(p2pDevices);

        // Configure the first TCP server application
        uint16_t tcpPort1 = startingPort++;
        Address serverAddr1(InetSocketAddress(ipInterfaces.GetAddress(1), tcpPort1));
        PacketSinkHelper tcpServerHelper1("ns3::TcpSocketFactory", serverAddr1);
        ApplicationContainer serverApp1 = tcpServerHelper1.Install(networkNodes.Get(1));
        serverApp1.Start(Seconds(0.0));  // Start server immediately
        serverApp1.Stop(Seconds(simDuration));

        // Configure the first TCP client application
        BulkSendHelper tcpClientHelper1("ns3::TcpSocketFactory", serverAddr1);
        tcpClientHelper1.SetAttribute("MaxBytes", UintegerValue(0));  // Unlimited data
        tcpClientHelper1.SetAttribute("SendSize", UintegerValue(pktSizeBytes));
        ApplicationContainer clientApp1 = tcpClientHelper1.Install(networkNodes.Get(0));
        clientApp1.Start(Seconds(1.0));  // Slight delay before starting the client
        clientApp1.Stop(Seconds(simDuration));

        // Configure the second TCP server application
        uint16_t tcpPort2 = startingPort++;
        Address serverAddr2(InetSocketAddress(ipInterfaces.GetAddress(1), tcpPort2));
        PacketSinkHelper tcpServerHelper2("ns3::TcpSocketFactory", serverAddr2);
        ApplicationContainer serverApp2 = tcpServerHelper2.Install(networkNodes.Get(1));
        serverApp2.Start(Seconds(0.0));  // Start server immediately
        serverApp2.Stop(Seconds(simDuration));

        // Configure the second TCP client application
        BulkSendHelper tcpClientHelper2("ns3::TcpSocketFactory", serverAddr2);
        tcpClientHelper2.SetAttribute("MaxBytes", UintegerValue(0));  // Unlimited data
        tcpClientHelper2.SetAttribute("SendSize", UintegerValue(pktSizeBytes));
        ApplicationContainer clientApp2 = tcpClientHelper2.Install(networkNodes.Get(0));
        clientApp2.Start(Seconds(1.5));  // Slightly later start for the second client
        clientApp2.Stop(Seconds(simDuration));

        // Install the Flow Monitor to gather statistics
        FlowMonitorHelper flowMonitorHelper;
        Ptr<FlowMonitor> monitor = flowMonitorHelper.InstallAll();

        // Start the simulation
        Simulator::Stop(Seconds(simDuration));
        Simulator::Run();

        // Analyze and print throughput data for each flow
        monitor->CheckForLostPackets();
        std::map<FlowId, FlowMonitor::FlowStats> flowStatistics = monitor->GetFlowStats();
        for (const auto &flow : flowStatistics)
        {
            double throughputMbps = (flow.second.rxBytes * 8.0) / simDuration / 1e6;  // Convert to Mbps
            std::cout << "Latency: " << delay << " - Flow ID: " << flow.first
                      << " Throughput: " << throughputMbps << " Mbps" << std::endl;
        }

        // Clean up simulation state
        Simulator::Destroy();
    }

    return 0;
}
