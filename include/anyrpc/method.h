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

#ifndef ANYRPC_METHOD_H_
#define ANYRPC_METHOD_H_

namespace anyrpc
{
//! Function pointer for executing a method.  The params are the inputs and the result is the output.
typedef void Function (Value& params, Value& result);

class MethodManager;

//! The Method class is used to specify RPC functions to call.
/*!
 *  Most methods will be functions that independently process the params to produce the result.
 *  These methods are typically based on MethodFunction and are allocated when added.  These need
 *  to be deleted when removed.
 *
 *  It is also possible to have Methods that are derived from this class
 *  that do no need to be delete since they are statically defined.
 *  MethodInternal classes are a typical example.
 */
class ANYRPC_API Method
{
public:
    Method(std::string const& name, std::string const& help, bool deleteOnRemove=true) :
        name_(name), help_(help), deleteOnRemove_(deleteOnRemove) {}
    virtual ~Method() {}

    virtual void Execute(Value& /* params */, Value& /* result */) {}
    std::string& Name() { return name_; }
    std::string& Help() { return help_; }
    bool DeleteOnRemove() { return deleteOnRemove_; }

protected:
    std::string name_;
    std::string help_;
    bool deleteOnRemove_;

    log_define("AnyRcp.Method");
};

//! A MethodFunction is created with a function pointer that is call by the Execute method.
class MethodFunction : public Method
{
public:
    MethodFunction(Function* function, std::string const& name, std::string const& help, bool deleteOnRemove=true) :
        Method(name, help, deleteOnRemove), function_(function) {}
    virtual void Execute(Value& params, Value& result) { if (function_) function_(params, result); }
private:
    Function *function_;
};

//! MethodInternal classes are typically used for introspection of the MethodManager.
class MethodInternal : public Method
{
public:
    MethodInternal(MethodManager* manager, std::string const& name, std::string const& help, bool deleteOnRemove=true) :
        Method(name,help,deleteOnRemove), manager_(manager) {}
protected:
    MethodManager *manager_;
};

//! The ListMethod class is used to return a list of all of the defined methods.
class ListMethod : public MethodInternal
{
public:
    ListMethod(MethodManager* manager, std::string const& name, std::string const& help) :
        MethodInternal(manager,name,help) {}
    virtual void Execute(Value& params, Value& result);
};

//! The HelpMethod class is used to return the help string for a specific method.
class HelpMethod : public MethodInternal
{
public:
    HelpMethod(MethodManager* manager, std::string const& name, std::string const& help) :
        MethodInternal(manager,name,help) {}
    virtual void Execute(Value& params, Value& result);
};

// Introspection support
static const std::string LIST_METHODS("system.listMethods");
static const std::string LIST_METHODS_HELP("List all methods available on a server as an array of strings");
static const std::string METHOD_HELP("system.methodHelp");
static const std::string METHOD_HELP_HELP("Retrieve the help string for a named method");

//! The MethodManager holds the list of methods to execute.
/*!
 *  Both functions and method can be added to the list of executable methods.
 *  A method can be executed by name.
 */
class ANYRPC_API MethodManager
{
public:
    MethodManager();

    ~MethodManager();

    void AddFunction(Function* function, std::string const& name, std::string const& help);
    void AddMethod(Method* method);
    bool ExecuteMethod(std::string const& name, Value& params, Value& result);
    void ListMethods(Value& params, Value& result);
    void FindHelpMethod(Value& params, Value& result);

private:
    typedef std::map<std::string, Method*> MethodMap;   //!< definition of mapping function using the method name as the key
    MethodMap methods_;                                 //!< map of method names to method definitions

    log_define("AnyRPC.MethodManager");
};

} // namespace anyrpc

#endif // ANYRPC_METHOD_H_
