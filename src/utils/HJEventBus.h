#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

NS_HJ_BEGIN

template <typename... Args>
class HJEventBus
{
private:
	using CallbackType = std::function<void(Args...)>;
	std::unordered_map<std::string, std::vector<std::weak_ptr<CallbackType>>> topicSubscribers;
	mutable std::mutex mutex_;
public:
	template<typename Callback>
	std::shared_ptr<CallbackType> subscribe(const std::string& topic, Callback&& callback)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto callbackPtr = std::make_shared<CallbackType>(std::forward<Callback>(callback));
		topicSubscribers[topic].emplace_back(callbackPtr); // save weak_ptr
		return callbackPtr; // return shared_ptr, the caller should hold a reference to it
	}

	void subscribe(const std::string& topic, std::shared_ptr<CallbackType> callbackptr)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		topicSubscribers[topic].emplace_back(std::move(callbackptr));
	}

	void publish(const std::string& topic, Args... args)
	{
		std::vector<std::shared_ptr<CallbackType>> activeCallbacks;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			auto it = topicSubscribers.find(topic);
			if (it == topicSubscribers.end())
			{
				return;
			}
			auto& callbacks = it->second;
			auto callbackIt = callbacks.begin();

			while (callbackIt != callbacks.end())
			{
				if (auto callbackPtr = callbackIt->lock()) { // try weak_ptr to shared_ptr
					//(*callbackPtr)(args...); //not in lock
					activeCallbacks.push_back(std::move(callbackPtr));
					++callbackIt;
				}
				else
				{
					// remove invalid weak_ptr
					callbackIt = callbacks.erase(callbackIt);
				}
			}
		}
		for (auto& callbackPtr : activeCallbacks)
		{
			(*callbackPtr)(args...);
		}
	}
};

NS_HJ_END