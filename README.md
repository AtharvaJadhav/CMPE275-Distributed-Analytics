# Distributed Analytics

Setting Up json.hpp
Download the nlohmann/json Library - https://github.com/nlohmann/json

Download the single-header file json.hpp from the repository.
Save json.hpp in your project directory.
Include json.hpp in Your Project:

#include "json.hpp"

setting up asio.hpp

Download the Standalone asio Headers - https://github.com/chriskohlhoff/asio

Download the standalone asio headers and place them in your project directory.
Include asio in Your Project:

#include "asio.hpp


## Compilation Commands

Compile the Metadata Analytics Server:
g++ -std=c++11 -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/asio -o metadata_analytics_server /Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/metadata_analytics_server.cpp

Compile the Analytics Server:
g++ -std=c++11 -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/asio -o analytics_server /Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/analytics_server.cpp

Compile the Decoy Registry Server:
g++ -std=c++11 -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/asio -o decoy_registry_server /Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/decoy_registry_server.cpp

Compile the Dummy Ingestion Client:
g++ -std=c++11 -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics -I/Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/asio -o dummy_ingestion_client /Users/atharva/Downloads/275/CMPE275-Distributed-Analytics/dummy_ingestion_client.cpp

Running the Servers

Run the Decoy Registry Server:
./decoy_registry_server

Run the First Analytics Server:
./analytics_server 192.168.1.3 12346

Run the Second Analytics Server:
./analytics_server 192.168.1.4 12347

Run the Metadata Analytics Server:
./metadata_analytics_server

Run the Dummy Ingestion Client:
./dummy_ingestion_client

Expected Output

Decoy Registry Server Terminal:

Node connected: {"Ip":"192.168.1.2","computingCapacity":0.8,"nodeType":"metadata Analytics"}

Node connected: {"Ip":"192.168.1.3","computingCapacity":0.6,"nodeType":"analytics"}

Node connected: {"Ip":"192.168.1.4","computingCapacity":0.6,"nodeType":"analytics"}

First Analytics Server Terminal:
Init Analytics received. Replicas: 192.168.1.3 192.168.1.4
Analytics request received with ID: 1
Data: 1 
Data: 2 
Data: 3 

Second Analytics Server Terminal:
Init Analytics received. Replicas: 192.168.1.3 192.168.1.4

Metadata Analytics Server Terminal:
Node: 192.168.1.2, Type: metadata Analytics, Capacity: 0.8
Node: 192.168.1.3, Type: analytics, Capacity: 0.6
Node: 192.168.1.4, Type: analytics, Capacity: 0.6
Metadata Analytics Leader: 192.168.1.2
Metadata Ingestion Leader: 192.168.1.3
Init Election Ingestion: 192.168.1.104
Sent analytics request to 192.168.1.3 with request ID: 1
Acknowledgment received for request ID: 1

Dummy Ingestion Client Terminal:

No output expected (unless you add logging), but the client should successfully send data.
