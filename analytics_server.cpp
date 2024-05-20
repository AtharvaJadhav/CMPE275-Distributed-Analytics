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

std::vector<std::vector<std::string>> storedData; // To store ingested data

void handleInitAnalytics(const std::string &message)
{
    try
    {
        json initAnalyticsMessage = json::parse(message);

        if (initAnalyticsMessage["requestType"] == "Init Analytics")
        {
            if (!initAnalyticsMessage.contains("Replicas") || !initAnalyticsMessage["Replicas"].is_array())
            {
                throw std::runtime_error("Invalid or missing 'Replicas' key in Init Analytics message");
            }

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

            // Process the data
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

            // Send acknowledgment
            asio::io_context io_context;
            tcp::resolver resolver(io_context);
            tcp::resolver::results_type endpoints = resolver.resolve("10.0.0.65", "12458");

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
                // QUERY 0: Maximum of the averages AQI over all areas and all timelines
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
                // QUERY 1: Maximum of the maximum AQIs over all time over all the areas
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

            // Send query response
            asio::io_context io_context;
            tcp::resolver resolver(io_context);
            tcp::resolver::results_type endpoints = resolver.resolve("10.0.0.65", "12460");

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
        std::thread(handleClient, std::move(socket)).detach();
    }
}

void registerWithRegistryServer(const std::string &serverIp, unsigned short port, const std::string &nodeIp, double computingCapacity)
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
            {"nodeType", "analytics"},
            {"computingCapacity", computingCapacity}};

        std::string message = registrationRequest.dump() + "\n";
        asio::write(socket, asio::buffer(message));

        socket.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <IP_ADDRESS> <PORT>" << std::endl;
        return 1;
    }

    std::string nodeIp = argv[1];
    unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));
    double computingCapacity = 0.6; // Example capacity

    // Register with registry server
    registerWithRegistryServer("10.0.0.65", 12345, nodeIp, computingCapacity);

    // Start server to handle Init Analytics messages and analytics requests
    try
    {
        asio::io_context io_context;
        std::thread serverThread([&io_context, port]()
                                 { startServer(io_context, port); });

        serverThread.join();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in server: " << e.what() << std::endl;
    }

    return 0;
}
