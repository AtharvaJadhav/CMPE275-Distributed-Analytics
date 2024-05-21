#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <numeric>
#include <asio.hpp>
#include "json.hpp"

using json = nlohmann::json;
using asio::ip::tcp;

std::vector<std::vector<std::string>> storedData;
std::vector<std::string> analyticsNodes;
int currentNodeIndex = 0;

void handleInitAnalytics(const std::string &message);
void handleClient(tcp::socket socket);
void startServer(asio::io_context &io_context, unsigned short port);

void handleNodeDiscovery(const std::string &message)
{
    try
    {
        std::cout << "Received node discovery message: " << message << std::endl; // Log the received message

        json discoveryMessage = json::parse(message);

        if (discoveryMessage["requestType"] == "Node Discovery")
        {
            std::vector<json> nodes = discoveryMessage["nodes"];
            std::string metadataAnalyticsLeader = discoveryMessage["metadataAnalyticsLeader"];
            std::string metadataIngestionLeader = discoveryMessage["metadataIngestionLeader"];
            std::string initElectionIngestion = discoveryMessage["initElectionIngestion"];

            for (const auto &node : nodes)
            {
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
    catch (const json::exception &e)
    {
        std::cerr << "JSON Exception: " << e.what() << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
}

void handleInitAnalytics(const std::string &message)
{
    try
    {
        json initAnalyticsMessage = json::parse(message);

        std::cout << "Received message: " << message << std::endl; // Log received message

        if (initAnalyticsMessage["requestType"] == "Init Analytics")
        {
            std::vector<std::string> replicas = initAnalyticsMessage["Replicas"];
            std::cout << "Init Analytics received. Replicas: ";
            for (const auto &replica : replicas)
            {
                std::cout << replica << " ";
            }
            std::cout << std::endl;
        }
        else if (initAnalyticsMessage["requestType"] == "analytics")
        {
            int requestId = initAnalyticsMessage["requestID"];
            std::cout << "Analytics request received with ID: " << requestId << std::endl;

            std::vector<std::vector<std::string>> data = initAnalyticsMessage["Data"];
            storedData.insert(storedData.end(), data.begin(), data.end());

            for (const auto &item : data)
            {
                std::cout << "Data: ";
                for (const auto &value : item)
                {
                    std::cout << value << " ";
                }
                std::cout << std::endl;
            }

            asio::io_context io_context;
            tcp::resolver resolver(io_context);
            tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "12458");

            tcp::socket socket(io_context);
            asio::connect(socket, endpoints);

            json acknowledgment = {
                {"requestType", "analytics acknowledgment"},
                {"requestID", requestId}};

            std::string ackMessage = acknowledgment.dump() + "\n";
            asio::write(socket, asio::buffer(ackMessage));

            socket.close();
        }
        else if (initAnalyticsMessage["requestType"] == "query")
        {
            int requestId = initAnalyticsMessage["requestID"];
            int queryType = initAnalyticsMessage["query"];
            std::cout << "Query request received with ID: " << requestId << " and query type: " << queryType << std::endl;

            std::string maxArea;
            double maxValue;

            if (queryType == 0)
            {
                std::unordered_map<std::string, std::vector<double>> areaData;
                for (const auto &item : storedData)
                {
                    std::string area = item[9];
                    double aqi = std::stod(item[4]);
                    areaData[area].push_back(aqi);
                }

                double maxAverage = 0.0;
                for (const auto &[area, values] : areaData)
                {
                    double sum = std::accumulate(values.begin(), values.end(), 0.0);
                    double average = sum / values.size();
                    if (average > maxAverage)
                    {
                        maxAverage = average;
                        maxArea = area;
                    }
                }
                maxValue = maxAverage;
            }
            else if (queryType == 1)
            {
                std::unordered_map<std::string, double> areaMaxData;
                for (const auto &item : storedData)
                {
                    std::string area = item[9];
                    double aqi = std::stod(item[4]);
                    if (areaMaxData.find(area) == areaMaxData.end() || aqi > areaMaxData[area])
                    {
                        areaMaxData[area] = aqi;
                    }
                }

                double maxAqi = 0.0;
                for (const auto &[area, max] : areaMaxData)
                {
                    if (max > maxAqi)
                    {
                        maxAqi = max;
                        maxArea = area;
                    }
                }
                maxValue = maxAqi;
            }

            asio::io_context io_context;
            tcp::resolver resolver(io_context);
            tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "12460");

            tcp::socket socket(io_context);
            asio::connect(socket, endpoints);

            json queryResponse = {
                {"requestType", "query response"},
                {"requestID", requestId},
                {"maxArea", maxArea}};
            if (queryType == 0)
            {
                queryResponse["maxAverage"] = maxValue;
            }
            else
            {
                queryResponse["maxAqi"] = maxValue;
            }

            std::string responseMessage = queryResponse.dump() + "\n";
            asio::write(socket, asio::buffer(responseMessage));

            std::cout << "Sent query response with request ID: " << requestId << " max area: " << maxArea << " max value: " << maxValue << std::endl;

            socket.close();
        }
        else
        {
            std::cerr << "Invalid request type: " << initAnalyticsMessage["requestType"] << std::endl;
        }
    }
    catch (const json::exception &e)
    {
        std::cerr << "JSON Exception: " << e.what() << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
}

void handleClient(tcp::socket socket)
{
    try
    {
        asio::streambuf buffer;
        asio::read_until(socket, buffer, "\n");
        std::istream is(&buffer);
        std::string message;
        std::getline(is, message);

        std::cout << "Received message: " << message << std::endl; // Log received message

        handleInitAnalytics(message);

        socket.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in client handling: " << e.what() << std::endl;
    }
}

void startServer(asio::io_context &io_context, unsigned short port)
{
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    while (true)
    {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Accepted connection from: " << socket.remote_endpoint() << std::endl; // Log connection acceptance
        std::thread(handleClient, std::move(socket)).detach();
    }
}

void registerWithRegistryServer(double computingCapacity)
{
    try
    {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve("192.168.1.102", "12351");

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        json registrationRequest = {
            {"requestType", "registering"},
            {"Ip", "192.168.1.2"}, // Update this IP address as needed
            {"nodeType", "metadata Analytics"},
            {"computingCapacity", computingCapacity}};

        std::string message = registrationRequest.dump() + "\n";
        asio::write(socket, asio::buffer(message));

        std::cout << "Sent registration request to registry server" << std::endl; // Log registration request

        asio::streambuf response;
        asio::read_until(socket, response, "\n");
        std::istream is(&response);
        std::string responseMessage;
        std::getline(is, responseMessage);

        handleNodeDiscovery(responseMessage);

        socket.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception while registering with registry server: " << e.what() << std::endl;
    }
}

int main()
{
    double computingCapacity = 0.8;

    registerWithRegistryServer(computingCapacity);

    try
    {
        asio::io_context io_context;
        std::thread serverThread([&io_context]()
                                 { startServer(io_context, 12457); });

        serverThread.join();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in server: " << e.what() << std::endl;
    }

    return 0;
}
