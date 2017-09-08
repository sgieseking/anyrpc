
#include "anyrpc/anyrpc.h"
#include <iostream>
#include <fstream>
#include <cctype>

using namespace std;
using namespace anyrpc;

char myString[] = "my very long String";

log_define("");

char hexDigits[] = "0123456789abcdef";

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
    value["double"] = -5.53276283923e-6;
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
    cout << "Xml data to stdout: " << endl;
    cout << ToXmlString(value,12,true);
    //XmlWriter xmlWriter(stdoutStream,true);
    //xmlWriter.SetScientificPrecision(6);
    //xmlWriter << value;
    cout << endl;

    // Write data to a string
    WriteStringStream strStream;
    XmlWriter xmlStrWriter(strStream);
    xmlStrWriter << value;

    const char *xml = strStream.GetBuffer();
    size_t xmlLength = strStream.Length();
    cout << "Xml data in string:" << endl << xml << endl;

    // Use string data and parse in place.
    // This will modify the string and reference value strings directly from the input string.
    InSituStringStream sstream((char*)xml, xmlLength);
    XmlReader xmlreader(sstream);
    xmlreader >> doc;
    if (xmlreader.HasParseError())
    {
        cout << "Parse Error: " << xmlreader.GetParseErrorStr();
        cout << "; at position " << xmlreader.GetErrorOffset() << endl;
    }
    else
    {
        // Display value using internal converter - close to Json format but not exact
        cout << "Value traversal after reading: " << endl;
        cout << doc.GetValue() << endl;
    }

    return 0;
}
