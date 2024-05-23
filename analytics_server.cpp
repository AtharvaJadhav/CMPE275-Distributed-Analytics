#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <numeric>
#include <QTcpSocket>
#include <QTcpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

std::vector<std::vector<std::string>> storedData; // To store ingested data

void handleInitAnalytics(const std::string &message)
{
    try
    {
        QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(message).toUtf8());
        QJsonObject initAnalyticsMessage = doc.object();

        if (initAnalyticsMessage["requestType"].toString() == "Init Analytics")
        {
            if (!initAnalyticsMessage.contains("Replicas") || !initAnalyticsMessage["Replicas"].isArray())
            {
                throw std::runtime_error("Invalid or missing 'Replicas' key in Init Analytics message");
            }

            QJsonArray replicas = initAnalyticsMessage["Replicas"].toArray();
            std::cout << "Init Analytics received. Replicas: ";
            for (const auto &replica : replicas)
            {
                std::cout << replica.toString().toStdString() << " ";
            }
            std::cout << std::endl;
        }
        else if (initAnalyticsMessage["requestType"].toString() == "analytics")
        {
            int requestId = initAnalyticsMessage["requestID"].toInt();
            std::cout << "Analytics request received with ID: " << requestId << std::endl;

            // Process the data
            QJsonArray dataArray = initAnalyticsMessage["Data"].toArray();
            for (const auto &dataValue : dataArray)
            {
                std::vector<std::string> dataRow;
                QJsonArray dataItem = dataValue.toArray();
                for (const auto &item : dataItem)
                {
                    dataRow.push_back(item.toString().toStdString());
                }
                storedData.push_back(dataRow);
            }

            for (const auto &item : storedData)
            {
                std::cout << "Data: ";
                for (const auto &value : item)
                {
                    std::cout << value << " ";
                }
                std::cout << std::endl;
            }

            // Send acknowledgment
            QTcpSocket socket;
            socket.connectToHost("10.0.0.65", 12458);
            if (socket.waitForConnected())
            {
                QJsonObject acknowledgment;
                acknowledgment["requestType"] = "analytics acknowledgment";
                acknowledgment["requestID"] = requestId;

                QJsonDocument ackDoc(acknowledgment);
                std::string ackMessage = ackDoc.toJson(QJsonDocument::Compact).toStdString() + "\n";
                socket.write(ackMessage.c_str());
                socket.waitForBytesWritten();
                socket.close();
            }
        }
        else if (initAnalyticsMessage["requestType"].toString() == "query")
        {
            int requestId = initAnalyticsMessage["requestID"].toInt();
            int queryType = initAnalyticsMessage["query"].toInt();
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
            QTcpSocket socket;
            socket.connectToHost("10.0.0.65", 12460);
            if (socket.waitForConnected())
            {
                QJsonObject queryResponse;
                queryResponse["requestType"] = "query response";
                queryResponse["requestID"] = requestId;
                queryResponse["maxArea"] = QString::fromStdString(maxArea);
                if (queryType == 0)
                {
                    queryResponse["maxAverage"] = maxValue;
                }
                else
                {
                    queryResponse["maxAqi"] = maxValue;
                }

                QJsonDocument responseDoc(queryResponse);
                std::string responseMessage = responseDoc.toJson(QJsonDocument::Compact).toStdString() + "\n";
                socket.write(responseMessage.c_str());
                socket.waitForBytesWritten();

                std::cout << "Sent query response with request ID: " << requestId << " max area: " << maxArea << " max value: " << maxValue << std::endl;

                socket.close();
            }
        }
        else
        {
            std::cerr << "Invalid request type: " << initAnalyticsMessage["requestType"].toString().toStdString() << std::endl;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
}

void handleClient(QTcpSocket *socket)
{
    try
    {
        socket->waitForReadyRead();
        QByteArray buffer = socket->readAll();
        std::string message = buffer.toStdString();

        handleInitAnalytics(message);

        socket->close();
        delete socket;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in client handling: " << e.what() << std::endl;
    }
}

void startServer(quint16 port)
{
    QTcpServer server;
    if (!server.listen(QHostAddress::Any, port))
    {
        std::cerr << "Server could not start!" << std::endl;
    }
    else
    {
        std::cout << "Server started!" << std::endl;
    }

    QObject::connect(&server, &QTcpServer::newConnection, [&server]()
    {
        QTcpSocket *socket = server.nextPendingConnection();
        std::thread(handleClient, socket).detach();
    });

    QEventLoop loop;
    loop.exec();
}

void registerWithRegistryServer(const std::string &serverIp, unsigned short port, const std::string &nodeIp, double computingCapacity)
{
    try
    {
        QTcpSocket socket;
        socket.connectToHost(QString::fromStdString(serverIp), port);
        if (socket.waitForConnected())
        {
            QJsonObject registrationRequest;
            registrationRequest["requestType"] = "registering";
            registrationRequest["Ip"] = QString::fromStdString(nodeIp);
            registrationRequest["nodeType"] = "analytics";
            registrationRequest["computingCapacity"] = computingCapacity;

            QJsonDocument doc(registrationRequest);
            std::string message = doc.toJson(QJsonDocument::Compact).toStdString() + "\n";
            socket.write(message.c_str());
            socket.waitForBytesWritten();
            socket.close();
        }
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
        startServer(port);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in server: " << e.what() << std::endl;
    }

    return 0;
}
