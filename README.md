# Distributed Analytics Setup and Execution Guide

## Setup

### Prerequisites

- C++11 or later
- [nlohmann/json](https://github.com/nlohmann/json) library
- [Standalone Asio](https://github.com/chriskohlhoff/asio) library

### Download and Place Libraries

1. **nlohmann/json Library**:
   - Download the single-header file `json.hpp` from the [nlohmann/json repository](https://github.com/nlohmann/json).
   - Save `json.hpp` in your project directory.

2. **Standalone Asio Library**:
   - Download the standalone Asio headers from the [Asio repository](https://github.com/chriskohlhoff/asio).
   - Place the Asio headers in your project directory.

### Include Libraries in Your Project

```cpp
#include "json.hpp"
#include "asio.hpp"
```

### Project Directory Structure

```
/project_directory
|-- analytics_server.cpp
|-- decoy_registry_server.cpp
|-- dummy_ingestion_client.cpp
|-- metadata_analytics_server.cpp
|-- json.hpp
|-- asio/ (containing Asio headers)
```

## Compilation Commands

Compile each component using the following commands:

### Metadata Analytics Server

```sh
g++ -std=c++11 -I/project_directory -I/project_directory/asio -o metadata_analytics_server /project_directory/metadata_analytics_server.cpp
```

### Analytics Server

```sh
g++ -std=c++11 -I/project_directory -I/project_directory/asio -o analytics_server /project_directory/analytics_server.cpp
```

### Decoy Registry Server

```sh
g++ -std=c++11 -I/project_directory -I/project_directory/asio -o decoy_registry_server /project_directory/decoy_registry_server.cpp
```

### Dummy Ingestion Client

```sh
g++ -std=c++11 -I/project_directory -I/project_directory/asio -o dummy_ingestion_client /project_directory/dummy_ingestion_client.cpp
```

## Running the Servers

1. **Run the Decoy Registry Server**:

    ```sh
    ./decoy_registry_server
    ```

2. **Run the First Analytics Server**:

    ```sh
    ./analytics_server 192.168.1.3 12346
    ```

3. **Run the Second Analytics Server**:

    ```sh
    ./analytics_server 192.168.1.4 12347
    ```

4. **Run the Metadata Analytics Server**:

    ```sh
    ./metadata_analytics_server
    ```

5. **Run the Dummy Ingestion Client**:

    ```sh
    ./dummy_ingestion_client
    ```

## Expected Output

### Decoy Registry Server Terminal:

```
Node connected: {"Ip":"192.168.1.2","computingCapacity":0.8,"nodeType":"metadata Analytics"}
Node connected: {"Ip":"192.168.1.3","computingCapacity":0.6,"nodeType":"analytics"}
Node connected: {"Ip":"192.168.1.4","computingCapacity":0.6,"nodeType":"analytics"}
```

### First Analytics Server Terminal:

```
Init Analytics received. Replicas: 192.168.1.3 192.168.1.4
Analytics request received with ID: 1
Data: 1 
Data: 2 
Data: 3
```

### Second Analytics Server Terminal:

```
Init Analytics received. Replicas: 192.168.1.3 192.168.1.4
```

### Metadata Analytics Server Terminal:

```
Node: 192.168.1.2, Type: metadata Analytics, Capacity: 0.8
Node: 192.168.1.3, Type: analytics, Capacity: 0.6
Node: 192.168.1.4, Type: analytics, Capacity: 0.6
Metadata Analytics Leader: 192.168.1.2
Metadata Ingestion Leader: 192.168.1.3
Init Election Ingestion: 192.168.1.104
Sent analytics request to 192.168.1.3 with request ID: 1
Acknowledgment received for request ID: 1
Sent query request to 10.0.0.65 with request ID: 1 and query type: 0
Sent query request to 10.0.0.65 with request ID: 2 and query type: 1
Query response received for request ID: 1 Max Area: Crescent City, Max Value: 18.7
Query response received for request ID: 2 Max Area: Crescent City, Max Value: 20.1
```

### Dummy Ingestion Client Terminal:

```
Sent ingestion data to 10.0.0.65 on port 12459
2020-08-10T01:00@0 41.75613 -124.20347 PM2.5 17.3 UG/M3 18.0 62 2 Crescent City North Coast Unified Air Quality Management District 840060150007 840060150007
2020-08-10T02:00@1 41.75613 -124.20347 PM2.5 20.1 UG/M3 20.0 60 3 Crescent City North Coast Unified Air Quality Management District 840060150007 840060150007
Sent query request to 10.0.0.65 on port 12459 with request ID: 1 and query type: 0
Sent query request to 10.0.0.65 on port 12459 with request ID: 2 and query type: 1
```
