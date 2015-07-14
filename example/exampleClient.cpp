
#include "anyrpc/anyrpc.h"

#if defined(ANYRPC_THREADING)
#include <thread>
#endif

using namespace std;
using namespace anyrpc;

log_define("main");

/*
 *  This application should be used with exampleServer.
 *  The parameters to the application are strictly positions so that a
 *  command line parser in not needed.
 *
 *  Param 1 : client type - defaults to jsonhttp
 *  Param 2 : IpAddress for the server - defaults to 127.0.0.1
 *  Param 3 : Port for the server - defaults to 6000
 *  Param 4 : Number of clients - only useful if threading is enabled
 *
 *  When testing with multiple clients, you should start the server with
 *  a multithreaded version or the client access will be serialized.
 */

void TestClient(Client* client, char* ipAddress, int port)
{
    Value params;
    Value result;
    bool success;

    client->SetServer(ipAddress, port);
    client->SetTimeout(2000);

    // Add the parameters
    // Note that the parameters will be invalidated inside the Call
    params[0] = 5;
    params[1] = 6;
    success = client->Call("add", params, result);
    cout << "success: " << success << ", add:      " << result << endl;

    // Subtract the parameters
    success = client->Call("subtract", params, result);
    cout << "success: " << success << ", subtract: " << result << endl;

    // Multiply the parameters
    success = client->Call("multiply", params, result);
    cout << "success: " << success << ", multiply: " << result << endl;

    // Call divide but this is not implement so expect a failure
    success = client->Call("divide", params, result);
    cout << "success: " << success << ", divide:   " << result << endl;

    // Post calls to add
    for (int i=0; i<5; i++)
    {
        params.SetArray();
        params[0] = i;
        params[1] = 12;
        success = client->Post("add", params, result);
    }
    // Now get the results
    for (int i=0; i<5; i++)
    {
        success = client->GetPostResult(result);
        cout << "success: " << success << ", add:      " << result << endl;
    }

    // Call multiply as a notification - no result will be returned
    params.SetArray();
    params[0] = 10;
    params[1] = 12;
    success = client->Notify("multiply", params, result);
    cout << "Notify: success: " << success << ", multiply: " << result << endl;

    // Call method that will wait for the indicated number of milliseconds before returning
    params.SetArray();
    params[0].SetInt(1000);
    success = client->Call("wait", params, result);
    cout << "success: " << success << ", wait:   " << result << endl;
}

int main(int argc, char *argv[])
{
    // Start the logger up first so all other code can be logged
    InitializeLogger();

    // Log the time for the system running
    log_time(INFO);

    // Default to one client but allow up to 4;
    const int maxClients = 6;
    int numClients = 1;
    Client* client[maxClients];

    // Determine the number of clients
    if (argc > 4)
        numClients = atoi(argv[4]);
    if (numClients > maxClients)
        numClients = maxClients;

    // Determine the client type and allocate
    for (int i=0; i<numClients; i++)
    {
        if (argc < 2)
        {
            cout << "Protocol must be defined" << endl;
            return 1;
        }
#if defined(ANYRPC_INCLUDE_JSON)
        else if (strcasecmp(argv[1], "jsonhttp") == 0)
            client[i] = new JsonHttpClient;
        else if (strcasecmp(argv[1], "jsontcp") == 0)
            client[i] = new JsonTcpClient;
#endif
#if defined(ANYRPC_INCLUDE_XML)
        else if (strcasecmp(argv[1], "xmlhttp") == 0)
            client[i] = new XmlHttpClient;
        else if (strcasecmp(argv[1], "xmltcp") == 0)
            client[i] = new XmlTcpClient;
#endif
#if defined(ANYRPC_INCLUDE_MESSAGEPACK)
        else if (strcasecmp(argv[1], "messagepackhttp") == 0)
            client[i] = new MessagePackHttpClient;
        else if (strcasecmp(argv[1], "messagepacktcp") == 0)
            client[i] = new MessagePackTcpClient;
#endif
        else
        {
            cout << "Undefined protocol0" << endl;
            return 1;
        }
    }

    // Determine the IP Address
    char* ipAddress = (char*)"127.0.0.1";
    if (argc > 2)
        ipAddress = argv[2];

    // Determine the port
    int port = 9000;
    if (argc > 3)
        port = atoi(argv[3]);

#if defined(ANYRPC_THREADING)
    std::thread clientThread[maxClients];

    // start each of the clients with a short delay between them
    for (int i=0; i<numClients; i++)
    {
        clientThread[i] = std::thread(&TestClient, client[i], ipAddress, port);
        this_thread::sleep_for( chrono::milliseconds(100) );
    }

    // join all of the threads as they finish
    for (int i=0; i<numClients; i++)
        clientThread[i].join();
#else
    // Test a single client
    TestClient(client[0], ipAddress, port);
#endif

    // delete clients
    for (int i=0; i<numClients; i++)
        delete client[i];

    return 0;
}
