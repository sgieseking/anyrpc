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
    mutex_.lock();
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
    mutex_.lock();
    MethodMap::const_iterator it = methods_.find(name);
    if (it == methods_.end())
    {
        // not found so add new method
        methods_[name] = new MethodFunction(function,name,help);
        mutex_.unlock();
    }
    else
    {
        mutex_.unlock();
        // function already defined, throw exception
        // the user can catch and ignore the exception if this behavior is desired
        anyrpc_throw(AnyRpcErrorFunctionRedefine, "Attempt to redefine function name: " + name);
    }
}

void MethodManager::AddMethod(Method* method)
{
    mutex_.lock();
    MethodMap::const_iterator it = methods_.find(method->Name());
    if (it == methods_.end())
    {
        // not found so add new method
        methods_[method->Name()] = method;
        mutex_.unlock();
    }
    else
    {
        mutex_.unlock();
        // method already defined, clean up then throw exception
        // the user can catch and ignore the exception if this behavior is desired
        if (method->DeleteOnRemove())
            delete method;
        anyrpc_throw(AnyRpcErrorMethodRedefine, "Attempt to redefine method name: " + method->Name());
    }
}

bool MethodManager::RemoveMethod(std::string const& name)
{
    mutex_.lock();
	MethodMap::const_iterator it = methods_.find(name);
	if (it == methods_.end())
	{
	    mutex_.unlock();
        return false; // no such method exists
	}
	Method *method = it->second;
	if (method->ActiveThreads() > 0)
	{
	    method->SetDelayedRemove();
	}
	else
	{
        if (method->DeleteOnRemove())
            delete method;  // free the method pointer data
        methods_.erase(it);
	}
	mutex_.unlock();
	return true;
}

bool MethodManager::ExecuteMethod(std::string const& name, Value& params, Value& result)
{
    mutex_.lock();
    MethodMap::const_iterator it = methods_.find(name);
    if ((it == methods_.end()) || it->second->DelayedRemove())
    {
        mutex_.unlock();
        return false;
    }
    // Add this thread to the list of users for this method
    Method *method = it->second;
    method->AddThread();
    mutex_.unlock();

    it->second->Execute(params,result);

    mutex_.lock();
    // Finish with this thread using this method
    method->RemoveThread();
    // Check if we should remove the method - must have no active threads
    if (method->DelayedRemove() && (method->ActiveThreads() == 0))
    {
        // Find the method again in case other changes to the list have occurred
        it = methods_.find(name);
        if (it == methods_.end())
        {
            mutex_.unlock();
            anyrpc_throw(AnyRpcErrorInternalError, "Method not found for delayed remove: " + name);
        }
        if (method->DeleteOnRemove())
            delete method;  // free the method pointer data
        methods_.erase(it);
    }

    mutex_.unlock();

    return true;
}

void MethodManager::ListMethods(Value& params, Value& result)
{
    int i=0;
    result.SetArray();
    mutex_.lock();
    result.SetSize(methods_.size());
    for (MethodMap::const_iterator it = methods_.begin(); it != methods_.end(); ++it)
        result[i++] = it->first;
    mutex_.unlock();
}

void MethodManager::FindHelpMethod(Value& params, Value& result)
{
    if (!params.IsArray() || (params.Size() != 1) || !params[0].IsString())
        anyrpc_throw(AnyRpcErrorInvalidParams, "Invalid parameters");

    mutex_.lock();
    MethodMap::const_iterator it = methods_.find(params[0].GetString());
    if (it == methods_.end())
    {
        mutex_.unlock();
        anyrpc_throw(AnyRpcErrorMethodNotFound, "Unknown method name: " + std::string(params[0].GetString()));
    }

    result = it->second->Help();
    mutex_.unlock();
}

}
