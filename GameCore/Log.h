/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace /* anonymous */ {

	template<typename T>
	std::stringstream & _LogToStream(std::stringstream & ss, T&& t)
	{
		ss << std::forward<T>(t) << std::endl;
		return ss;
	}

	template<typename T, typename... TArgs>
	std::stringstream & _LogToStream(std::stringstream & ss, T&& t, TArgs&&... args)
	{
		ss << std::forward<T>(t);
		return _LogToStream(ss, std::forward<TArgs>(args)...);
	}

    template<typename T>
    void _LogToNothing(T&& /*t*/)
    {
    }

    template<typename T, typename... TArgs>
    void _LogToNothing(T&& /*t*/, TArgs&&... args)
    {
        _LogToNothing(std::forward<TArgs>(args)...);
    }

}

class Logger
{
public:

	Logger()
		: mCurrentListener()
		, mStoredMessages()
        , mStoredMessagesMutex()
	{
	}

	Logger(Logger const &) = delete;
	Logger(Logger &&) = delete;
	Logger & operator=(Logger const &) = delete;
	Logger & operator=(Logger &&) = delete;

	void RegisterListener(
		std::function<void(std::string const & message)> listener)
	{
		assert(!mCurrentListener);
		mCurrentListener = std::move(listener);

		// Publish all the messages so far
        {
            std::scoped_lock lock(mStoredMessagesMutex);

            for (std::string const & message : mStoredMessages)
            {
                mCurrentListener(message);
            }
        }
	}

	void UnregisterListener()
	{
		assert(!!mCurrentListener);
		mCurrentListener = {};
	}

	template<typename...TArgs>
	void Log(TArgs&&... args)
	{
		std::stringstream ss;
		_LogToStream(ss, std::forward<TArgs>(args)...);

		std::string const & message = ss.str();

		// Store
        {
            std::scoped_lock lock(mStoredMessagesMutex);

            mStoredMessages.push_back(message);
            if (mStoredMessages.size() > MaxStoredMessages)
            {
                mStoredMessages.pop_front();
            }
        }

		// Publish
		if (!!mCurrentListener)
		{
			mCurrentListener(message);
		}

        // Output
        std::cout << message << std::endl;
	}

    template<typename...TArgs>
    void LogToNothing(TArgs&&... args)
    {
        _LogToNothing(std::forward<TArgs>(args)...);
    }

public:

	static Logger Instance;

private:

	// The current listener
	std::function<void(std::string const & message)> mCurrentListener;

	// The messages stored so far
	std::deque<std::string> mStoredMessages;
	static constexpr size_t MaxStoredMessages = 1000;

    // The mutex for the message queue
    std::mutex mStoredMessagesMutex;
};

//
// Global aliases
//

template<typename... TArgs>
void LogMessage(TArgs&&... args)
{
	Logger::Instance.Log(std::forward<TArgs>(args)...);
}

template<typename... TArgs>
void LogDebug(TArgs&&... args)
{
#ifdef _DEBUG
	Logger::Instance.Log(std::forward<TArgs>(args)...);
#else
    Logger::Instance.LogToNothing(std::forward<TArgs>(args)...);
#endif
}

