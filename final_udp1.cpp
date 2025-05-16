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
    // Simulation parameters: can be adjusted for testing different scenarios.
    uint32_t pktSizeBytes = 1024;               // Size of packets sent (in bytes).
    double simDuration = 10.0;                  // Duration of each simulation (seconds).
    std::string linkRate = "5Mbps";             // Bandwidth for the Point-to-Point link.
    std::vector<std::string> delayOptions = {   // Latency options to test (milliseconds).
        "10ms", "50ms", "100ms", "200ms", "500ms"};

    // Allow customization of linkRate from the command line.
    CommandLine cmd;
    cmd.AddValue("linkRate", "Data rate for the Point-to-Point link", linkRate);
    cmd.Parse(argc, argv);

    // Start TCP servers with incremented base port values.
    uint16_t startingPort = 9000;

    // Iterate over different latencies to evaluate network performance.
    for (const std::string &delay : delayOptions)
    {
        // 1. **Node Setup**: Create two nodes (source and destination).
        NodeContainer networkNodes;
        networkNodes.Create(2);

        // Install the internet stack (TCP/IP protocol stack) on both nodes.
        InternetStackHelper internetHelper;
        internetHelper.Install(networkNodes);

        // 2. **Point-to-Point Link Configuration**: 
        // Set the data rate and latency for the connection.
        PointToPointHelper p2pHelper;
        p2pHelper.SetDeviceAttribute("DataRate", StringValue(linkRate));
        p2pHelper.SetChannelAttribute("Delay", StringValue(delay));

        // Establish the Point-to-Point link between the nodes.
        NetDeviceContainer p2pDevices = p2pHelper.Install(networkNodes);

        // Assign IP addresses to the interfaces of the nodes.
        Ipv4AddressHelper ipHelper;
        ipHelper.SetBase("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer ipInterfaces = ipHelper.Assign(p2pDevices);

        // 3. **Application Setup**: Configure servers and clients.

        // Configure the first TCP server on Node 2.
        uint16_t tcpPort1 = startingPort++;
        Address serverAddr1(InetSocketAddress(ipInterfaces.GetAddress(1), tcpPort1));
        PacketSinkHelper tcpServerHelper1("ns3::TcpSocketFactory", serverAddr1);
        ApplicationContainer serverApp1 = tcpServerHelper1.Install(networkNodes.Get(1));
        serverApp1.Start(Seconds(0.0));
        serverApp1.Stop(Seconds(simDuration));

        // Configure the first TCP client on Node 1.
        BulkSendHelper tcpClientHelper1("ns3::TcpSocketFactory", serverAddr1);
        tcpClientHelper1.SetAttribute("MaxBytes", UintegerValue(0));  // Unlimited data.
        tcpClientHelper1.SetAttribute("SendSize", UintegerValue(pktSizeBytes));
        ApplicationContainer clientApp1 = tcpClientHelper1.Install(networkNodes.Get(0));
        clientApp1.Start(Seconds(1.0));
        clientApp1.Stop(Seconds(simDuration));

        // Configure the second TCP server on Node 2.
        uint16_t tcpPort2 = startingPort++;
        Address serverAddr2(InetSocketAddress(ipInterfaces.GetAddress(1), tcpPort2));
        PacketSinkHelper tcpServerHelper2("ns3::TcpSocketFactory", serverAddr2);
        ApplicationContainer serverApp2 = tcpServerHelper2.Install(networkNodes.Get(1));
        serverApp2.Start(Seconds(0.0));
        serverApp2.Stop(Seconds(simDuration));

        // Configure the second TCP client on Node 1.
        BulkSendHelper tcpClientHelper2("ns3::TcpSocketFactory", serverAddr2);
        tcpClientHelper2.SetAttribute("MaxBytes", UintegerValue(0));  // Unlimited data.
        tcpClientHelper2.SetAttribute("SendSize", UintegerValue(pktSizeBytes));
        ApplicationContainer clientApp2 = tcpClientHelper2.Install(networkNodes.Get(0));
        clientApp2.Start(Seconds(1.5));  // Delay to stagger flows.
        clientApp2.Stop(Seconds(simDuration));

        // 4. **Flow Monitoring**: Install the Flow Monitor to track statistics.
        FlowMonitorHelper flowMonitorHelper;
        Ptr<FlowMonitor> monitor = flowMonitorHelper.InstallAll();

        // Start and stop the simulation.
        Simulator::Stop(Seconds(simDuration));
        Simulator::Run();

        // 5. **Results Analysis**: Collect and print throughput metrics.
        monitor->CheckForLostPackets();
        std::map<FlowId, FlowMonitor::FlowStats> flowStatistics = monitor->GetFlowStats();
        for (const auto &flow : flowStatistics)
        {
            double throughputMbps = (flow.second.rxBytes * 8.0) / simDuration / 1e6;  // Convert to Mbps.
            std::cout << "Latency: " << delay << " - Flow ID: " << flow.first
                      << " Throughput: " << throughputMbps << " Mbps" << std::endl;
        }

        // 6. **Cleanup**: Destroy the simulation objects.
        Simulator::Destroy();
    }

    return 0;
}
