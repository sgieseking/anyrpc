
#include "anyrpc/anyrpc.h"
#include "anyrpc/logger.h"
#include <iostream>
#include <fstream>

#if defined(BUILD_WITH_MSGPACK_LIBRARY)
#include <msgpack.h>
#endif

using namespace std;
using namespace anyrpc;

log_define("main");

#if defined(BUILD_WITH_MSGPACK_LIBRARY)
void unpack(char const* buf, size_t len)
{
    /* buf is allocated by client. */
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_unpack_return ret;
    int i = 0;
    msgpack_unpacked_init(&result);
    ret = msgpack_unpack_next(&result, buf, len, &off);
    while (ret == MSGPACK_UNPACK_SUCCESS)
    {
        msgpack_object obj = result.data;

        /* Use obj. */
        cout << "Map number " << i++ << endl;
        msgpack_object_print(stdout, obj);
        cout << endl;
        /* If you want to allocate something on the zone, you can use zone. */
        /* msgpack_zone* zone = result.zone; */
        /* The lifetime of the obj and the zone,  */

        ret = msgpack_unpack_next(&result, buf, len, &off);
    }
    msgpack_unpacked_destroy(&result);

    if (ret == MSGPACK_UNPACK_CONTINUE)
    {
        cout << "All msgpack_object in the buffer is consumed." << endl;
    }
    else if (ret == MSGPACK_UNPACK_PARSE_ERROR)
    {
        cout << "The data in the buf is invalid format." << endl;
    }
}
#endif
char hexDigits[] = "0123456789abcdef";

int main()
{
    // Start the logger up first so all other code can be logged
    InitializeLogger();

    // Log the time for the system running
    log_time(INFO);

    char binFile[] = "test.bin";
    Value value;
    Document doc;

    // Create map with various values of different types
    value["integer"] = 32;
    value["string"] = "test string\nsecond line";
    value["double"] = 5.532e-5;
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

    // Write data to a file so that it can be checked by the messagepack library
    WriteFileStream fstream(binFile);
    MessagePackWriter writer(fstream);
    writer << value;
    // Close the file so all data is written out of the buffers
    fstream.Close();

#if defined(BUILD_WITH_MSGPACK_LIBRARY)
    // Check that all of the data is read back properly using the messagepack library
    cout << "Read back using MessagePack library:" << endl;
    FILE* fp = fopen(binFile,"r");
    char readBuffer[65536];
    size_t length = fread(readBuffer, 1, sizeof(readBuffer), fp);
    unpack(readBuffer, length);
#endif

    // Read using this libraries messagepack reader
    cout << "Read back using MessagePack reader" << endl;
    ReadFileStream fstream3(binFile);
    MessagePackReader msgpackreader(fstream3);
    msgpackreader >> doc;

#if defined(ANYRPC_INCLUDE_JSON)
    // Easier to verify by looking at the Json format
    cout << "Data in Json format: " << endl;
    JsonWriter jsonwriter(stdoutStream);
    jsonwriter << doc.GetValue();
    cout << endl;
#else
    cout << "Data: " << endl;
    cout << doc.GetValue() << endl;
#endif

    // Write to a string to verify in situ parsing
    cout << "Check InSitu Parsing" << endl;
    WriteStringStream wstream;
    MessagePackWriter swriter(wstream);
    swriter << value;

    // Read back from the string stream
    const char* mPack = wstream.GetBuffer();
    size_t mPacklength = wstream.Length();
    InSituStringStream rstream((char*)mPack, mPacklength);
    MessagePackReader insituReader(rstream);
    insituReader >> doc;

    if (insituReader.HasParseError())
        cout << "Parse Error: " << insituReader.GetParseErrorStr() << "; at position " << insituReader.GetErrorOffset() << endl;
    else
    {
        // Display value using internal converter - close to Json format but not exact
        cout << "Value traversal after reading: " << endl;
        cout << doc.GetValue() << endl;
    }

    return 0;
}
