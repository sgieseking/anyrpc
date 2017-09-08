
#include "anyrpc/anyrpc.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace anyrpc;

log_define("main");

int main()
{
    // Start the logger up first so all other code can be logged
    InitializeLogger();

    // Log the time for the executing the function
    log_time(INFO);

    Value value;
    Document doc;

    // Create map with various values of different types
    value["integer"] = 32;
    value["string"] = "test string\nsecond line";
    value["double"] = 5.532679123812e-5;
    value["dateTime"].SetDateTime(time(NULL));

    Value binary;
    char* binData = (char*)"\x0a\x0b\x0c\x0d\xff\x00\xee\xdd";
    binary.SetBinary( (unsigned char*)binData, 8);
    value.AddMember( "binary", binary );

    Value array;
    array.SetSize(4);
    array[0] = 5;
    array[1] = 10;
    array[2] = 15;
    array[3] = 20;
    value["array"] = array;

    // Write data to stdout
    cout << "Json data to stdout: " << endl;
    cout << ToJsonString(value,UTF8,8,true);
    //JsonWriter jsonWriter(stdoutStream,UTF8,true);
    //jsonWriter << value;
    cout << endl;

    // Write data to a string
    WriteStringStream strStream;
    JsonWriter jsonStrWriter(strStream);
    jsonStrWriter << value;

    const char *json = strStream.GetBuffer();
    size_t jsonLength = strStream.Length();
    cout << "Json data in string:" << endl << json << endl;

    // Use string data and parse in place.
    // This will modify the string and reference value strings directly from the input string.
    InSituStringStream sstream((char*)json, jsonLength);
    JsonReader jsonreader(sstream);
    jsonreader >> doc;
    if (jsonreader.HasParseError())
    {
        cout << "Parse Error: " << jsonreader.GetParseErrorStr();
        cout << "; at position " << jsonreader.GetErrorOffset() << endl;
    }
    else
    {
        // Display value using internal converter - close to Json format but not exact
        cout << "Value traversal after reading: " << endl;
        cout << doc.GetValue() << endl;
    }

    return 0;
}
