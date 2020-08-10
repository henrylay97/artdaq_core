#ifndef artdaq_core_Core_StatisticsCollection_hh
#define artdaq_core_Core_StatisticsCollection_hh

#include <boost/thread.hpp>
#include <map>
#include <memory>
#include <mutex>
#include "artdaq-core/Core/MonitoredQuantity.hh"

namespace artdaq {
/**
   * \brief A shared_ptr to a MonitoredQuantity instance
   */
typedef std::shared_ptr<MonitoredQuantity> MonitoredQuantityPtr;

/**
   * \brief A collection of MonitoredQuantity instances describing low-level statistics of the _artdaq_ system
   * 
   * A collection of MonitoredQuantity instances describing low-level statistics of the _artdaq_ system.
   * Periodically (default 1s) calculates statistics for each MonitoredQuantity instance.
   */
class StatisticsCollection
{
public:
	/**
	   * \brief Returns the singleton instance of the StatisticsCollection.
	   * \return StatisticsCollection instance.
	   */
	static StatisticsCollection& getInstance();

	/**
	   * \brief StatisticsCollection Destructor
	   */
	virtual ~StatisticsCollection() noexcept;

	/**
		 * \brief Registers a new MonitoredQuantity to be tracked by the StatisticsCollection
		 * \param name Name of the MonitoredQuantity (used for lookup)
		 * \param mqPtr shared_ptr to MonitoredQuantity
		 */
	void addMonitoredQuantity(const std::string& name,
	                          MonitoredQuantityPtr mqPtr);

	/**
		 * \brief Lookup and return a MonitoredQuantity from the StatisticsCollection
		 * \param name Name of the MonitoredQuantity
		 * \return MonitoredQuantityPtr (nullptr if not found in StatisticsCollection)
		 */
	MonitoredQuantityPtr getMonitoredQuantity(const std::string& name) const;

	/**
		 * \brief Reset all MonitoredQuantity object in this StatisticsCollection
		 */
	void reset();

	/**
		 * \brief Stops the statistics calculation thread
		 */
	void requestStop();

	/**
		 * \brief Start the background thread that performs MonitoredQuantity statistics calculation
		 */
	void run();

private:
	/**
	   * \brief Private constructor used by static getInstance()
	   */
	explicit StatisticsCollection();

	// disallow any copying
	/**
		 * \brief Deleted Copy Constructor
		 */
	StatisticsCollection(StatisticsCollection const&) = delete;  // not implemented
	/**
		 * \brief Deleted Copy-assignment Operator
		 */
	void operator=(StatisticsCollection const&) = delete;  // not implemented

	StatisticsCollection(StatisticsCollection&&) = delete;
	StatisticsCollection& operator=(StatisticsCollection&&) = delete;

	/**
		 * \brief Interval for calculation of statistics. Defaulted to 1 second
		 */
	double calculationInterval_{1.0};
	/**
		 * \brief Lookup map for MonitoredQuantityPtr objects, keyed by name
		 */
	std::map<std::string, MonitoredQuantityPtr> monitoredQuantityMap_;

	/**
		 * \brief Thread control varaible
		 */
	bool thread_stop_requested_;
	/**
		 * \brief Thread which performs calculation of MonitoredQuantity statistics
		 */
	std::unique_ptr<boost::thread> calculation_thread_;

	/**
		 * \brief Mutex for protecting accesses to the lookup map
		 */
	mutable std::mutex map_mutex_;
};
}  // namespace artdaq

#endif /* artdaq_core_Core_StatisticsCollection_hh */
