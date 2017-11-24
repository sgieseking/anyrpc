
#include "anyrpc/anyrpc.h"
#include "anyrpc/internal/time.h"

#if defined(__CYGWIN__)
# include <strings.h>
#endif

using namespace std;
using namespace anyrpc;

/*
 *  This application should be used with exampleClient.
 *  The parameters to the application are strictly positions so that a
 *  command line parser in not needed.
 *
 *  Param 1 : server type - defaults to jsonhttp
 *  Param 2 : Port for the server - defaults to 9000
 *  Param 3 : Number of seconds to wait until exiting
 *
 *  When testing with multiple clients, you should start the server with
 *  a multithreaded version or the client access will be serialized.
 */

log_define("root");

// Define the method functions that can be called
void Add(Value& params, Value& result)
{
    if ((!params.IsArray()) ||
        (params.Size() != 2) ||
        (!params[0].IsNumber()) ||
        (!params[1].IsNumber()))
        throw AnyRpcException(AnyRpcErrorInvalidParams, "Invalid parameters");
    result = params[0].GetDouble() + params[1].GetDouble();
}

void Subtract(Value& params, Value& result)
{
    if ((!params.IsArray()) ||
        (params.Size() != 2) ||
        (!params[0].IsNumber()) ||
        (!params[1].IsNumber()))
        throw AnyRpcException(AnyRpcErrorInvalidParams, "Invalid parameters");
    result = params[0].GetDouble() - params[1].GetDouble();
}

// Define a class method that could keep its own state variables
class Multiply : public Method
{
public:
    Multiply() :
        Method("multiply", "Multiply two numbers",false) {}
    virtual void Execute(Value& params, Value& result)
    {
        if ((!params.IsArray()) ||
            (params.Size() != 2) ||
            (!params[0].IsNumber()) ||
            (!params[1].IsNumber()))
            throw AnyRpcException(AnyRpcErrorInvalidParams, "Invalid parameters");
        result = params[0].GetDouble() * params[1].GetDouble();
    }
};

Multiply multiply;

// The Wait function will delay a given number of milliseconds before returning.
void Wait(Value& params, Value& result)
{
    if ((!params.IsArray()) ||
        (params.Size() != 1) ||
        (!params[0].IsNumber()))
        throw AnyRpcException(AnyRpcErrorInvalidParams, "Invalid parameters");
    int delay = params[0].GetInt();
    MilliSleep(delay);
    result.SetNull();
}

void Echo(Value& params, Value& result)
{
    result = params;
}


int main(int argc, char *argv[])
{
    // Start the logger up first so all other code can be logged
    InitializeLogger();

    // Log the time for the system running
    log_time(INFO);

    // Determine the type of server
    Server *server = 0;

    if (argc < 2)
        server = new AnyHttpServer;  // support all protocols
#if defined(ANYRPC_INCLUDE_JSON)
    else if (strcasecmp(argv[1], "jsonhttp") == 0)
        server = new JsonHttpServer;
    else if (strcasecmp(argv[1], "jsontcp") == 0)
        server = new JsonTcpServer;
#endif // defined(ANYRPC_INCLUDE_JSON)

#if defined(ANYRPC_INCLUDE_XML)
    else if (strcasecmp(argv[1], "xmlhttp") == 0)
        server = new XmlHttpServer;
    else if (strcasecmp(argv[1], "xmltcp") == 0)
        server = new XmlTcpServer;
#endif // defined(ANYRPC_INCLUDE_XML)

#if defined(ANYRPC_INCLUDE_MESSAGEPACK)
    else if (strcasecmp(argv[1], "messagepackhttp") == 0)
        server = new MessagePackHttpServer;
    else if (strcasecmp(argv[1], "messagepacktcp") == 0)
        server = new MessagePackTcpServer;
#endif // defined(ANYRPC_INCLUDE_MESSAGEPACK)

#if defined(ANYRPC_THREADING)
# if defined(ANYRPC_INCLUDE_JSON)
    else if (strcasecmp(argv[1], "jsonhttpmt") == 0)
        server = new JsonHttpServerMT;
    else if (strcasecmp(argv[1], "jsontcpmt") == 0)
        server = new JsonTcpServerMT;
    else if (strcasecmp(argv[1], "jsonhttptp") == 0)
        server = new JsonHttpServerTP;
    else if (strcasecmp(argv[1], "jsontcptp") == 0)
        server = new JsonTcpServerTP;
# endif // defined(ANYRPC_INCLUDE_JSON)

# if defined(ANYRPC_INCLUDE_XML)
    else if (strcasecmp(argv[1], "xmlhttpmt") == 0)
        server = new XmlHttpServerMT;
    else if (strcasecmp(argv[1], "xmltcpmt") == 0)
        server = new XmlTcpServerMT;
    else if (strcasecmp(argv[1], "xmlhttptp") == 0)
        server = new XmlHttpServerTP;
    else if (strcasecmp(argv[1], "xmltcptp") == 0)
        server = new XmlTcpServerTP;
# endif // defined(ANYRPC_INCLUDE_XML)

# if defined(ANYRPC_INCLUDE_MESSAGEPACK)
    else if (strcasecmp(argv[1], "messagepackhttpmt") == 0)
        server = new MessagePackHttpServerMT;
    else if (strcasecmp(argv[1], "messagepacktcpmt") == 0)
        server = new MessagePackTcpServerMT;
    else if (strcasecmp(argv[1], "messagepackhttptp") == 0)
        server = new MessagePackHttpServerTP;
    else if (strcasecmp(argv[1], "messagepacktcptp") == 0)
        server = new MessagePackTcpServerTP;
# endif // defined(ANYRPC_INCLUDE_MESSAGEPACK)

    else if (strcasecmp(argv[1], "anyhttptp") == 0)
        server = new AnyHttpServerTP; // support all protocols with thread pool server

#endif // defined(ANYRPC_THREADING)
    else
        server = new AnyHttpServer;  // support all protocols

    // Add the method calls
    MethodManager *methodManager = server->GetMethodManager();
    methodManager->AddFunction( &Add, "add", "Add two numbers");
    methodManager->AddFunction( &Subtract, "subtract", "Subtract two numbers");
    methodManager->AddMethod( &multiply );
    methodManager->AddFunction( &Wait, "wait", "Delay execution for a given number of milliseconds");
    methodManager->AddFunction( &Echo, "echo", "Return the same data that was sent");

    // Determine the port for the server
    int port = 9000;
    if (argc > 2)
        port = atoi(argv[2]);
    server->BindAndListen(port);

    // allow a large number of simultaneous connections
    server->SetMaxConnections(100);

    // Determine the time that the server will run
    int timeout = 30;
    if (argc > 3)
        timeout = atoi(argv[3]);

#if defined(ANYRPC_THREADING)
    // Run the server as a separate thread
    server->StartThread();
    this_thread::sleep_for( chrono::seconds(timeout) );
    server->StopThread();
#else
    // Run the server for a specified number of milliseconds
    server->Work(timeout * 1000);
#endif

    // clean up
    delete server;

    return 0;
}
