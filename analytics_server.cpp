#include <iostream>
#include <string>
#include <thread>
#include <asio.hpp>
#include "json.hpp"

using json = nlohmann::json;
using asio::ip::tcp;

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
        else
        {
            std::cerr << "Invalid request type: " << initAnalyticsMessage["requestType"] << std::endl;
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

void registerWithRegistryServer(const std::string &serverIp, unsigned short port, const std::string &nodeIp)
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
            {"computingCapacity", 0.7} // Example capacity
        };

        std::string message = registrationRequest.dump() + "\n";
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
    // Register with registry server
    registerWithRegistryServer("127.0.0.1", 12345, "192.168.1.3");

    // Start server to handle Init Analytics messages
    try
    {
        asio::io_context io_context;
        std::thread serverThread([&io_context]()
                                 { startServer(io_context, 12346); });

        serverThread.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in server: " << e.what() << std::endl;
    }

    return 0;
}
