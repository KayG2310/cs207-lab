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
    // Simulation parameters
    uint32_t packetSizeBytes = 1024;              // Packet size in bytes
    double simDurationSeconds = 10.0;            // Total simulation duration
    std::string linkDataRate = "5Mbps";          // Link data rate
    std::vector<std::string> linkLatencies = {   // Different latencies to simulate
        "10ms", "50ms", "100ms", "200ms", "500ms"};

    // Parse command-line arguments for customization
    CommandLine cmd;
    cmd.AddValue("linkDataRate", "Data rate of the link", linkDataRate);
    cmd.Parse(argc, argv);

    // Base port number for TCP server
    uint16_t baseTcpPort = 8080;

    // Loop through each latency value for testing
    for (const std::string &latency : linkLatencies)
    {
        // Create two nodes for communication
        NodeContainer nodes;
        nodes.Create(2);

        // Install the Internet stack on the nodes
        InternetStackHelper internet;
        internet.Install(nodes);

        // Setup Point-to-Point channel with specified latency
        PointToPointHelper p2pHelper;
        p2pHelper.SetDeviceAttribute("DataRate", StringValue(linkDataRate));
        p2pHelper.SetChannelAttribute("Delay", StringValue(latency));

        // Install devices on the Point-to-Point channel
        NetDeviceContainer devices = p2pHelper.Install(nodes);

        // Assign IP addresses to the devices
        Ipv4AddressHelper ipv4Helper;
        ipv4Helper.SetBase("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer ipInterfaces = ipv4Helper.Assign(devices);

        // Setup a TCP server on the second node
        uint16_t currentPort = baseTcpPort++; // Use a unique port for this iteration
        Address serverAddress(InetSocketAddress(ipInterfaces.GetAddress(1), currentPort));
        PacketSinkHelper tcpServer("ns3::TcpSocketFactory", serverAddress);
        ApplicationContainer serverApp = tcpServer.Install(nodes.Get(1));
        serverApp.Start(Seconds(0.0));         // Start server immediately
        serverApp.Stop(Seconds(simDurationSeconds));

        // Setup a TCP client on the first node
        BulkSendHelper tcpClient("ns3::TcpSocketFactory", serverAddress);
        tcpClient.SetAttribute("MaxBytes", UintegerValue(0));        // Unlimited data
        tcpClient.SetAttribute("SendSize", UintegerValue(packetSizeBytes));
        ApplicationContainer clientApp = tcpClient.Install(nodes.Get(0));
        clientApp.Start(Seconds(1.0));         // Start client after a short delay
        clientApp.Stop(Seconds(simDurationSeconds));

        // Enable Flow Monitor for tracking throughput
        FlowMonitorHelper flowMonitorHelper;
        Ptr<FlowMonitor> flowMonitor = flowMonitorHelper.InstallAll();

        // Run the simulation
        Simulator::Stop(Seconds(simDurationSeconds));
        Simulator::Run();

        // Analyze throughput from flow statistics
        flowMonitor->CheckForLostPackets();
        std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMonitor->GetFlowStats();

        for (auto const &flow : flowStats)
        {
            // Compute and display throughput for each flow
            double throughputMbps = (flow.second.rxBytes * 8.0) / simDurationSeconds / 1e6; // in Mbps
            std::cout << "Latency: " << latency << " - Flow ID: " << flow.first
                      << " Throughput: " << throughputMbps << " Mbps" << std::endl;
        }

        // Clean up simulation state
        Simulator::Destroy();
    }

    return 0;
}
