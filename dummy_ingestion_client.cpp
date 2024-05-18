#include <iostream>
#include <string>
#include <asio.hpp>
#include "json.hpp"

using json = nlohmann::json;
using asio::ip::tcp;

void sendIngestionData(const std::string &serverIp, unsigned short port, const std::vector<std::vector<int>> &data)
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

        socket.close();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main()
{
    // Example data to be ingested
    std::vector<std::vector<int>> data = {{1}, {2}, {3}};

    // Send data to metadata analytics server
    sendIngestionData("127.0.0.1", 12349, data); // Ensure this port matches the metadata analytics server ingestion port

    return 0;
}
