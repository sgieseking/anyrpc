// Copyright (C) 2015 SRG Technology, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ANYRPC_DOCUMENT_H_
#define ANYRPC_DOCUMENT_H_

namespace anyrpc
{

//! Create a document from the parsed stream of data
/*!
 *  The Document class creates an internal representation of the parsed data
 *  that can be accessed as Values.  The base of this document can be accessed
 *  from the GetValue() member function.
 *
 *  During the parsing process, a stack is used to keep track of the current
 *  chain of values (i.e. nested arrays and maps).
 *  The top of the stack is the current Value that is being modified.
 *
 *  AnyRPC allows extensions to the base representation for object types that
 *  are not part of the standard.  When written, these extensions use an
 *  array where the first element is the name of the extension and the
 *  following element contain the data.  For example, Json does not have a DateTime type
 *  so a two element array is used ["AnyRpcDateTime", "date"].  This data can
 *  be automatically converted while processing the data.
 */

class ANYRPC_API Document : public Handler
{
public:
    Document() : convertExtensions_(true) {}
    virtual ~Document() {}

    //!@name Document Member Functions
    //@{
    virtual void StartDocument();
    virtual void EndDocument();
    //@}

    //!@name Miscellaneous Member Functions
    //@{
    virtual void Null();

    virtual void BoolTrue();
    virtual void BoolFalse();

    virtual void DateTime(time_t dt);

    virtual void String(const char* str, std::size_t length, bool copy = true);
    virtual void Binary(const unsigned char* str, std::size_t length, bool copy = true);
    //@}

    //!@name Number Member Functions
    //@{
    virtual void Int(int i);
    virtual void Uint(unsigned u);
    virtual void Int64(int64_t i64);
    virtual void Uint64(uint64_t u64);
    virtual void Double(double d);
    //@}

    //!@name Map Processing Member Functions
    //@{
    virtual void StartMap();
    virtual void Key(const char* str, std::size_t length, bool copy = true);
    virtual void MapSeparator();
    virtual void EndMap(std::size_t memberCount = 0);
    //@}

    //!@name Array Processing Member Functions
    //@{
    virtual void StartArray();
    virtual void ArraySeparator();
    virtual void EndArray(std::size_t elementCount = 0);
    //@}

    //! Get the value representing the document data
    Value& GetValue() { return value_; }

    //! Set whether to automatically convert extensions to base types when parsing
    void ConvertExtensions(bool convert=true) { convertExtensions_ = convert; }

protected:
    log_define("AnyRPC.Doc");

private:
    //! Convert a string to a DateTime type
    void ConvertDateTime(Value *value);
    //! Convert a string in Base64 format to Binary
    void ConvertBase64(Value *value);

    Value value_;                   //!< Data for the document

    std::vector<Value*> stack_;     //!< Chain of values to the current position when parsing
    bool convertExtensions_;        //!< Whether to automatically convert extensions to base type
};

} // namespace anyrpc


#endif // ANYRPC_DOCUMENT_H_
