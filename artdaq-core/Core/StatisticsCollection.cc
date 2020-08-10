#include "artdaq-core/Core/StatisticsCollection.hh"
#include <iostream>
#include <utility>

namespace artdaq {
StatisticsCollection& StatisticsCollection::getInstance()
{
	static StatisticsCollection singletonInstance;
	return singletonInstance;
}

StatisticsCollection::StatisticsCollection()
{
	thread_stop_requested_ = false;
	try
	{
		calculation_thread_ = std::make_unique<boost::thread>(boost::bind(&StatisticsCollection::run, this));
	}
	catch (const boost::exception& e)
	{
		std::cerr << "Caught boost::exception starting Statistics Collection thread: " << boost::diagnostic_information(e) << ", errno=" << errno << std::endl;
		exit(5);
	}
}

StatisticsCollection::~StatisticsCollection() noexcept
{
	// stop and clean up the thread
	thread_stop_requested_ = true;

	try
	{
		// Having issues where ~StatisticsCollection is being called from within thread due to signal handlers
		if (calculation_thread_ && calculation_thread_->joinable() && calculation_thread_->get_id() != boost::this_thread::get_id())
		{
			calculation_thread_->join();
		}
	}
	catch (...)
	{
		// IGNORED
	}
}

void StatisticsCollection::
    addMonitoredQuantity(const std::string& name,
                         MonitoredQuantityPtr mqPtr)
{
	std::lock_guard<std::mutex> scopedLock(map_mutex_);
	monitoredQuantityMap_[name] = std::move(mqPtr);
}

MonitoredQuantityPtr
StatisticsCollection::getMonitoredQuantity(const std::string& name) const
{
	std::lock_guard<std::mutex> scopedLock(map_mutex_);
	MonitoredQuantityPtr emptyResult;
	std::map<std::string, MonitoredQuantityPtr>::const_iterator iter;
	iter = monitoredQuantityMap_.find(name);
	if (iter == monitoredQuantityMap_.end()) { return emptyResult; }
	return iter->second;
}

void StatisticsCollection::reset()
{
	std::lock_guard<std::mutex> scopedLock(map_mutex_);
	std::map<std::string, MonitoredQuantityPtr>::const_iterator iter;
	std::map<std::string, MonitoredQuantityPtr>::const_iterator iterEnd;
	iterEnd = monitoredQuantityMap_.end();
	for (iter = monitoredQuantityMap_.begin(); iter != iterEnd; ++iter)
	{
		iter->second->reset();
	}
}

void StatisticsCollection::requestStop()
{
	thread_stop_requested_ = true;
}

void StatisticsCollection::run()
{
	while (!thread_stop_requested_)
	{
		auto useconds = static_cast<uint64_t>(calculationInterval_ * 1000000);
		usleep(useconds);
		{
			std::lock_guard<std::mutex> scopedLock(map_mutex_);
			std::map<std::string, MonitoredQuantityPtr>::const_iterator iter;
			std::map<std::string, MonitoredQuantityPtr>::const_iterator iterEnd;
			iterEnd = monitoredQuantityMap_.end();
			for (iter = monitoredQuantityMap_.begin(); iter != iterEnd; ++iter)
			{
				iter->second->calculateStatistics();
			}
		}
	}
}
}  // namespace artdaq
