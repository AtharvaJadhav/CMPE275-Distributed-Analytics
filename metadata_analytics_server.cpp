#include <iostream>
#include <string>
#include <vector>
#include "json.hpp"
#include "asio.hpp"

using json = nlohmann::json;
using asio::ip::tcp;

enum class NodeType
{
    METADATA_ANALYTICS,
    ANALYTICS,
    METADATA_INGESTION,
    INGESTION
};

std::string nodeTypeToString(NodeType type)
{
    switch (type)
    {
    case NodeType::METADATA_ANALYTICS:
        return "metadata Analytics";
    case NodeType::ANALYTICS:
        return "analytics";
    case NodeType::METADATA_INGESTION:
        return "metadata Ingestion";
    case NodeType::INGESTION:
        return "ingestion";
    default:
        return "";
    }
}

void sendInitAnalytics(const std::string &analyticsIp, const std::vector<std::string> &replicas)
{
    try
    {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(analyticsIp, "12346");

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        json initAnalyticsMessage = {
            {"requestType", "Init Analytics"},
            {"Replicas", replicas}};

        std::string message = initAnalyticsMessage.dump() + "\n";
        asio::write(socket, asio::buffer(message));

        socket.close();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void handleNodeDiscovery(const std::string &message)
{
    try
    {
        json discoveryMessage = json::parse(message);

        if (discoveryMessage["requestType"] == "Node Discovery")
        {
            if (!discoveryMessage.contains("nodes") || !discoveryMessage["nodes"].is_array())
            {
                throw std::runtime_error("Invalid or missing 'nodes' key in Node Discovery message");
            }
            if (!discoveryMessage.contains("metadataAnalyticsLeader"))
            {
                throw std::runtime_error("Invalid or missing 'metadataAnalyticsLeader' key in Node Discovery message");
            }
            if (!discoveryMessage.contains("metadataIngestionLeader"))
            {
                throw std::runtime_error("Invalid or missing 'metadataIngestionLeader' key in Node Discovery message");
            }
            if (!discoveryMessage.contains("initElectionIngestion"))
            {
                throw std::runtime_error("Invalid or missing 'initElectionIngestion' key in Node Discovery message");
            }

            std::vector<json> nodes = discoveryMessage["nodes"];
            std::string metadataAnalyticsLeader = discoveryMessage["metadataAnalyticsLeader"];
            std::string metadataIngestionLeader = discoveryMessage["metadataIngestionLeader"];
            std::string initElectionIngestion = discoveryMessage["initElectionIngestion"];

            // Process the nodes
            for (const auto &node : nodes)
            {
                if (!node.contains("nodeType") || !node.contains("Ip") || !node.contains("computingCapacity"))
                {
                    std::cerr << "Invalid node data in Node Discovery message: " << node.dump() << std::endl;
                    continue;
                }

                std::string nodeType = node["nodeType"];
                std::string ip = node["Ip"];
                double computingCapacity = node["computingCapacity"];
                std::cout << "Node: " << ip << ", Type: " << nodeType << ", Capacity: " << computingCapacity << std::endl;

                // If it's an analytics node, send Init Analytics message
                if (nodeType == "analytics")
                {
                    std::vector<std::string> replicas = {"192.168.1.4", "192.168.1.5"}; // Example replicas
                    sendInitAnalytics(ip, replicas);
                }
            }

            std::cout << "Metadata Analytics Leader: " << metadataAnalyticsLeader << std::endl;
            std::cout << "Metadata Ingestion Leader: " << metadataIngestionLeader << std::endl;
            std::cout << "Init Election Ingestion: " << initElectionIngestion << std::endl;
        }
        else
        {
            std::cerr << "Invalid request type: " << discoveryMessage["requestType"] << std::endl;
        }
    }
    catch (json::exception &e)
    {
        std::cerr << "JSON Exception: " << e.what() << std::endl;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
}

void sendRegistration(const std::string &serverIp, unsigned short port, const std::string &nodeIp, NodeType nodeType, double computingCapacity)
{
    try
    {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(serverIp, std::to_string(port));

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        json registrationRequest = {
            {"requestType", "registering"},
            {"Ip", nodeIp},
            {"nodeType", nodeTypeToString(nodeType)},
            {"computingCapacity", computingCapacity}};

        std::string message = registrationRequest.dump() + "\n";
        asio::write(socket, asio::buffer(message));

        asio::streambuf response;
        asio::read_until(socket, response, "\n");
        std::istream is(&response);
        std::string responseMessage;
        std::getline(is, responseMessage);

        handleNodeDiscovery(responseMessage);

        socket.close();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main()
{
    // Example usage of sendRegistration function
    sendRegistration("127.0.0.1", 12345, "192.168.1.2", NodeType::METADATA_ANALYTICS, 0.8);

    return 0;
}
