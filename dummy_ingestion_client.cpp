#include <iostream>
#include <string>
#include <asio.hpp>
#include "json.hpp"

using json = nlohmann::json;
using asio::ip::tcp;

void sendIngestionData(const std::string &serverIp, unsigned short port, const std::vector<std::vector<std::string>> &data)
{
    try
    {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(serverIp, std::to_string(port));

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        json ingestionRequest = {
            {"requestType", "ingestion"},
            {"Data", data}};

        std::string message = ingestionRequest.dump() + "\n";
        asio::write(socket, asio::buffer(message));

        std::cout << "Sent ingestion data to " << serverIp << " on port " << port << std::endl;
        for (const auto &row : data)
        {
            for (const auto &item : row)
            {
                std::cout << item << " ";
            }
            std::cout << std::endl;
        }

        socket.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void sendQueryRequest(const std::string &serverIp, unsigned short port, int requestId, int queryType)
{
    try
    {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(serverIp, std::to_string(port));

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        json queryRequest = {
            {"requestType", "query"},
            {"requestID", requestId},
            {"query", queryType}};

        std::string message = queryRequest.dump() + "\n";
        asio::write(socket, asio::buffer(message));

        std::cout << "Sent query request to " << serverIp << " on port " << port << " with request ID: " << requestId << " and query type: " << queryType << std::endl;

        socket.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main()
{
    // Example data to be ingested
    std::vector<std::vector<std::string>> data = {
        {"2020-08-10T01:00@0", "41.75613", "-124.20347", "PM2.5", "17.3", "UG/M3", "18.0", "62", "2", "Crescent City", "North Coast Unified Air Quality Management District", "840060150007", "840060150007"},
        {"2020-08-10T02:00@1", "41.75613", "-124.20347", "PM2.5", "20.1", "UG/M3", "20.0", "60", "3", "Crescent City", "North Coast Unified Air Quality Management District", "840060150007", "840060150007"}};

    // Send data to metadata analytics server
    sendIngestionData("10.0.0.65", 12459, data); // Ensure this port matches the metadata analytics server ingestion port

    // Send query requests
    sendQueryRequest("10.0.0.65", 12459, 1, 0); // Query 0: Maximum of the averages AQI over all areas and all timelines
    sendQueryRequest("10.0.0.65", 12459, 2, 1); // Query 1: Maximum of the maximum AQIs over all time over all the areas

    return 0;
}
