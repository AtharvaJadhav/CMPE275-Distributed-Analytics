#include <iostream>
#include <vector>
#include <thread>
#include <QTcpSocket>
#include <QTcpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

std::vector<QJsonObject> registeredNodes;

void handleClient(QTcpSocket *socket)
{
    try
    {
        if (socket->waitForReadyRead())
        {
            QByteArray buffer = socket->readAll();
            std::string message = buffer.toStdString();

            std::cout << "Received message: " << message << std::endl; // Log received message

            QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(message).toUtf8());
            QJsonObject request = doc.object();

            if (request["requestType"].toString() == "registering")
            {
                QJsonObject nodeInfo;
                nodeInfo["Ip"] = request["Ip"].toString();
                nodeInfo["nodeType"] = request["nodeType"].toString();
                nodeInfo["computingCapacity"] = request["computingCapacity"].toDouble();
                registeredNodes.push_back(nodeInfo);
                std::cout << "Node connected: " << QJsonDocument(nodeInfo).toJson(QJsonDocument::Compact).toStdString() << std::endl;

                QJsonArray nodesArray;
                for (const auto &node : registeredNodes)
                {
                    nodesArray.append(node);
                }

                QJsonObject discoveryMessage;
                discoveryMessage["requestType"] = "Node Discovery";
                discoveryMessage["nodes"] = nodesArray;
                discoveryMessage["metadataAnalyticsLeader"] = "";
                discoveryMessage["metadataIngestionLeader"] = "";
                discoveryMessage["initElectionIngestion"] = "127.0.0.1";

                QJsonDocument discoveryDoc(discoveryMessage);
                std::string discoveryMessageStr = discoveryDoc.toJson(QJsonDocument::Compact).toStdString() + "\n";
                socket->write(discoveryMessageStr.c_str());
                socket->waitForBytesWritten();
            }
            else
            {
                std::cerr << "Received unknown request type: " << request["requestType"].toString().toStdString() << std::endl;
            }

            socket->close();
            delete socket;
        }
    }
    catch (std::exception &e)
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
        std::cout << "Accepted connection from: " << socket->peerAddress().toString().toStdString() << std::endl; // Log connection acceptance
        std::thread(handleClient, socket).detach();
    });

    QEventLoop loop;
    loop.exec();
}

int main()
{
    try
    {
        std::thread serverThread([]()
                                 { startServer(12345); });

        serverThread.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in server: " << e.what() << std::endl;
    }

    return 0;
}
