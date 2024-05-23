#include <iostream>
#include <string>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

void sendIngestionData(const std::string &serverIp, unsigned short port, const std::vector<std::vector<std::string>> &data)
{
    try
    {
        QTcpSocket socket;
        socket.connectToHost(QString::fromStdString(serverIp), port);
        if (socket.waitForConnected())
        {
            QJsonArray dataArray;
            for (const auto &row : data)
            {
                QJsonArray jsonRow;
                for (const auto &item : row)
                {
                    jsonRow.append(QString::fromStdString(item));
                }
                dataArray.append(jsonRow);
            }

            QJsonObject ingestionRequest;
            ingestionRequest["requestType"] = "ingestion";
            ingestionRequest["Data"] = dataArray;

            QJsonDocument doc(ingestionRequest);
            std::string message = doc.toJson(QJsonDocument::Compact).toStdString() + "\n";
            socket.write(message.c_str());
            socket.waitForBytesWritten();

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
        QTcpSocket socket;
        socket.connectToHost(QString::fromStdString(serverIp), port);
        if (socket.waitForConnected())
        {
            QJsonObject queryRequest;
            queryRequest["requestType"] = "query";
            queryRequest["requestID"] = requestId;
            queryRequest["query"] = queryType;

            QJsonDocument doc(queryRequest);
            std::string message = doc.toJson(QJsonDocument::Compact).toStdString() + "\n";
            socket.write(message.c_str());
            socket.waitForBytesWritten();

            std::cout << "Sent query request to " << serverIp << " on port " << port << " with request ID: " << requestId << " and query type: " << queryType << std::endl;

            socket.close();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main()
{
    std::vector<std::vector<std::string>> data = {
        {"2020-08-10T01:00@0", "41.75613", "-124.20347", "PM2.5", "17.3", "UG/M3", "18.0", "62", "2", "Crescent City", "North Coast Unified Air Quality Management District", "840060150007", "840060150007"},
        {"2020-08-10T02:00@1", "41.75613", "-124.20347", "PM2.5", "20.1", "UG/M3", "20.0", "60", "3", "Crescent City", "North Coast Unified Air Quality Management District", "840060150007", "840060150007"}};

    sendIngestionData("127.0.0.1", 12459, data);

    sendQueryRequest("127.0.0.1", 12459, 1, 0);
    sendQueryRequest("127.0.0.1", 12459, 2, 1);

    return 0;
}
