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

#include "anyrpc/api.h"
#include "anyrpc/logger.h"
#include "anyrpc/error.h"
#include "anyrpc/value.h"
#include "anyrpc/method.h"

namespace anyrpc
{

void ListMethod::Execute(Value& params, Value& result)
{
    if (manager_)
        manager_->ListMethods(params, result);
}

////////////////////////////////////////////////////////////////////////////////

void HelpMethod::Execute(Value& params, Value& result)
{
    if (manager_)
        manager_->FindHelpMethod(params, result);
}

////////////////////////////////////////////////////////////////////////////////

MethodManager::MethodManager()
{
    methods_[LIST_METHODS] = new ListMethod(this,LIST_METHODS,LIST_METHODS_HELP);
    methods_[METHOD_HELP] = new HelpMethod(this,METHOD_HELP,METHOD_HELP_HELP);
}

MethodManager::~MethodManager()
{
    MethodMap::iterator it = methods_.begin();
    for (MethodMap::iterator it = methods_.begin(); it != methods_.end(); it++)
    {
        if (it->second->DeleteOnRemove())
            delete it->second;  // free the method pointer data
    }
    methods_.clear();
}

void MethodManager::AddFunction(Function* function, std::string const& name, std::string const& help)
{
    MethodMap::const_iterator it = methods_.find(name);
    if (it == methods_.end())
        // not found so add new method
        methods_[name] = new MethodFunction(function,name,help);
}

void MethodManager::AddMethod(Method* method)
{
    MethodMap::const_iterator it = methods_.find(method->Name());
    if (it == methods_.end())
        // not found so add new method
        methods_[method->Name()] = method;
}

bool MethodManager::ExecuteMethod(std::string const& name, Value& params, Value& result)
{
    MethodMap::const_iterator it = methods_.find(name);
    if (it == methods_.end())
        return false;
    it->second->Execute(params,result);
    return true;
}

void MethodManager::ListMethods(Value& params, Value& result)
{
    int i=0;
    result.SetArray();
    result.SetSize(methods_.size());
    for (MethodMap::const_iterator it = methods_.begin(); it != methods_.end(); ++it)
        result[i++] = it->first;
}

void MethodManager::FindHelpMethod(Value& params, Value& result)
{
    if (!params.IsArray() || (params.Size() != 1) || !params[0].IsString())
        anyrpc_throw(AnyRpcErrorInvalidParams, "Invalid parameters");

    MethodMap::const_iterator it = methods_.find(params[0].GetString());
    if (it == methods_.end())
        anyrpc_throw(AnyRpcErrorMethodNotFound, "Unknown method name: " + std::string(params[0].GetString()));

    result = it->second->Help();
}

};
