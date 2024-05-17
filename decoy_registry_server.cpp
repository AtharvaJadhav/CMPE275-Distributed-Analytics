#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <asio.hpp>
#include "json.hpp"

using json = nlohmann::json;
using asio::ip::tcp;

std::vector<json> registeredNodes;

void handleClient(tcp::socket socket)
{
    try
    {
        asio::streambuf buffer;
        asio::read_until(socket, buffer, "\n");
        std::istream is(&buffer);
        std::string message;
        std::getline(is, message);

        json request = json::parse(message);
        if (request["requestType"] == "registering")
        {
            // Remove "requestType" key from the node's registration information
            json nodeInfo = {
                {"Ip", request["Ip"]},
                {"nodeType", request["nodeType"]},
                {"computingCapacity", request["computingCapacity"]}};
            registeredNodes.push_back(nodeInfo);
            std::cout << "Node connected: " << nodeInfo.dump() << std::endl;

            // Prepare and send node discovery message
            json discoveryMessage = {
                {"requestType", "Node Discovery"},
                {"nodes", registeredNodes},
                {"metadataAnalyticsLeader", ""},
                {"metadataIngestionLeader", ""},
                {"initElectionIngestion", "192.168.1.104"}};

            std::string discoveryMessageStr = discoveryMessage.dump() + "\n";
            asio::write(socket, asio::buffer(discoveryMessageStr));
        }
        else
        {
            std::cerr << "Received unknown request type: " << request["requestType"] << std::endl;
        }

        socket.close();
    }
    catch (std::exception &e)
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

int main()
{
    try
    {
        asio::io_context io_context;
        std::thread serverThread([&io_context]()
                                 { startServer(io_context, 12345); });

        serverThread.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in server: " << e.what() << std::endl;
    }

    return 0;
}
