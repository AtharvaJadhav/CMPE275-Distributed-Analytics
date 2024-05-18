#include <iostream>
#include <string>
#include <vector>
#include <asio.hpp>
#include "json.hpp"

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

std::vector<std::string> analyticsNodes;
int currentNodeIndex = 0;

void sendAnalyticsRequest(int requestId, const std::vector<std::vector<int>> &data)
{
    if (analyticsNodes.empty())
    {
        std::cerr << "No analytics nodes available." << std::endl;
        return;
    }

    // Select the next analytics node in a round-robin fashion
    std::string analyticsIp = analyticsNodes[currentNodeIndex];
    currentNodeIndex = (currentNodeIndex + 1) % analyticsNodes.size();

    try
    {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        std::cout << "Resolving " << analyticsIp << " on port 12346" << std::endl;
        tcp::resolver::results_type endpoints = resolver.resolve(analyticsIp, "12346");

        tcp::socket socket(io_context);
        std::cout << "Connecting to " << analyticsIp << std::endl;
        asio::connect(socket, endpoints);

        json analyticsRequest = {
            {"requestType", "analytics"},
            {"requestID", requestId},
            {"Data", data}};

        std::string message = analyticsRequest.dump() + "\n";
        asio::write(socket, asio::buffer(message));

        std::cout << "Sent analytics request to " << analyticsIp << " with request ID: " << requestId << std::endl;

        socket.close();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception while connecting to " << analyticsIp << ": " << e.what() << std::endl;
    }
}

void handleAnalyticsAcknowledgment(const std::string &message)
{
    try
    {
        json acknowledgment = json::parse(message);

        if (acknowledgment["requestType"] == "analytics acknowledgment")
        {
            int requestId = acknowledgment["requestID"];
            std::cout << "Acknowledgment received for request ID: " << requestId << std::endl;
        }
        else
        {
            std::cerr << "Invalid request type: " << acknowledgment["requestType"] << std::endl;
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

            // Collect analytics nodes
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

                if (nodeType == "analytics")
                {
                    analyticsNodes.push_back(ip);
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

void handleIngestionData(const std::string &message)
{
    try
    {
        json ingestionMessage = json::parse(message);

        if (ingestionMessage["requestType"] == "ingestion")
        {
            std::vector<std::vector<int>> data = ingestionMessage["Data"];
            static int requestId = 1;

            // Send analytics request
            sendAnalyticsRequest(requestId++, data);
        }
        else
        {
            std::cerr << "Invalid request type: " << ingestionMessage["requestType"] << std::endl;
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
        std::cerr << "Exception while registering with registry server: " << e.what() << std::endl;
    }
}

int main()
{
    // Example usage of sendRegistration function
    sendRegistration("127.0.0.1", 12345, "192.168.1.2", NodeType::METADATA_ANALYTICS, 0.8);

    // For simplicity, we'll handle acknowledgments and ingestion data in the main thread
    try
    {
        asio::io_context io_context;

        // Thread for acknowledgments
        std::thread ackThread([&io_context]()
                              {
            tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12348));
            while (true) {
                tcp::socket socket(io_context);
                acceptor.accept(socket);

                asio::streambuf buffer;
                asio::read_until(socket, buffer, "\n");
                std::istream is(&buffer);
                std::string message;
                std::getline(is, message);

                handleAnalyticsAcknowledgment(message);
                socket.close();
            } });

        // Thread for ingestion data
        std::thread ingestionThread([&io_context]()
                                    {
            tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12349));
            while (true) {
                tcp::socket socket(io_context);
                acceptor.accept(socket);

                asio::streambuf buffer;
                asio::read_until(socket, buffer, "\n");
                std::istream is(&buffer);
                std::string message;
                std::getline(is, message);

                handleIngestionData(message);
                socket.close();
            } });

        ackThread.join();
        ingestionThread.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in main thread: " << e.what() << std::endl;
    }

    return 0;
}
