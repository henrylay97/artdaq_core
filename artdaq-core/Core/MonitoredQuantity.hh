#ifndef artdaq_core_Core_MonitoredQuantity_hh
#define artdaq_core_Core_MonitoredQuantity_hh

#include <boost/thread/mutex.hpp>

#include <math.h>
#include <stdint.h>
#include <vector>

namespace artdaq {
/**
	* \brief struct containing MonitoredQuantity data
	*/
struct MonitoredQuantityStats
{
	typedef double DURATION_T;    ///< A Duration
	typedef double TIME_POINT_T;  ///< A point in time

	/**
		* \brief Which data points to return (all or only recent)
		*/
	enum class DataSetType
	{
		FULL = 0,   ///< the full data set (all samples)
		RECENT = 1  ///< recent data only
	};

	long long fullSampleCount;     ///< The total number of samples represented
	double fullSampleRate;         ///< The total number of samples over the full duration of sampling
	double fullValueSum;           ///< The sum of all samples
	double fullValueSumOfSquares;  ///< The sum of the squares of all samples
	double fullValueAverage;       ///< The average of all samples
	double fullValueRMS;           ///< The RMS of all samples
	double fullValueMin;           ///< The smallest value of all samples
	double fullValueMax;           ///< The largest value of all sampels
	double fullValueRate;          ///< The sum of all samples over the full duration of sampling
	DURATION_T fullDuration;       ///< The full duration of sampling

	long long recentSampleCount;     ///< The number of samples in the "recent" time window
	double recentSampleRate;         ///< The number of samples in the "recent" time window, divided by the length of that window
	double recentValueSum;           ///< The sum of the "recent" samples
	double recentValueSumOfSquares;  ///< The sum of the squares of the "recent" samples
	double recentValueAverage;       ///< The average of the "recent" samples
	double recentValueRMS;           ///< The RMS of the 'recent" samples
	double recentValueMin;           ///< The smallest value of the "recent" samples
	double recentValueMax;           ///< The largest value of the "recent" samples
	double recentValueRate;          ///< The sum of the "recent" samples, divided by the length of the "recent" time window
	DURATION_T recentDuration;       ///< The length of the "recent" time window

	std::vector<long long> recentBinnedSampleCounts;  ///< Sample counts for each instance of calculateStatistics in _intervalForRecentStats (rolling window)
	std::vector<double> recentBinnedValueSums;        ///< Sums for each instance of calculateStatistics in _intervalForRecentStats (rolling window)
	std::vector<DURATION_T> recentBinnedDurations;    ///< Duration between each instance of calcualteStatistics in _intervalForRecentStats (rolling window)
	std::vector<TIME_POINT_T> recentBinnedEndTimes;   ///< Last sample time in each instance of calculateStatistics in _intervalForRecentStats (rolling window)

	double lastSampleValue;            ///< Value of the most recent sample
	double lastValueRate;              ///< Latest rate point (sum of values over calculateStatistics interval)
	TIME_POINT_T lastCalculationTime;  ///< Last time calculateStatistics was called
	bool enabled;                      ///< Whether the MonitoredQuantity is collecting data

	/**
		 * \brief Returns the sample count for the requested interval
		 * \param t Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The sample count for the requested interval
		 */
	long long getSampleCount(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentSampleCount : fullSampleCount; }

	/**
		 * \brief Returns the sum of values in the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The sum of values in the requested interval
		 */
	double getValueSum(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentValueSum : fullValueSum; }

	/**
		 * \brief Returns the average of the values in the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The average of the values in the requested interval
		 */
	double getValueAverage(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentValueAverage : fullValueAverage; }

	/**
		 * \brief Returns the sum of the values in the requested interval, divided by the duration of the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The sum of the values in the requested interval, divided by the duration of the requested interval
		 */
	double getValueRate(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentValueRate : fullValueRate; }

	/**
		 * \brief Returns the RMS of the values in the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The RMS of the values in the requested interval
		 */
	double getValueRMS(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentValueRMS : fullValueRMS; }

	/**
		 * \brief Returns the smallest of the values in the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The smallest of the values in the requested interval
		 */
	double getValueMin(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentValueMin : fullValueMin; }

	/**
		 * \brief Returns the largest of the values in the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The largest of the values in the requested interval
		 */
	double getValueMax(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentValueMax : fullValueMax; }

	/**
		 * \brief Returns the duration of the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The duration of the requested interval
		 */
	DURATION_T getDuration(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentDuration : fullDuration; }

	/**
		 * \brief Returns the sample rate in the requested interval
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return The sample rate in the requested interval
		 */
	double getSampleRate(DataSetType t = DataSetType::FULL) const { return t == DataSetType::RECENT ? recentSampleRate : fullSampleRate; }

	/**
		 * \brief 
		 * \param t  Which interval to return, DataSetType::FULL (default) or DataSetType::RECENT
		 * \return 
		 */
	double getSampleLatency(DataSetType t = DataSetType::FULL) const
	{
		auto v = getSampleRate(t);
		return v ? 1e6 / v : INFINITY;
	}

	/**
		 * \brief Accessor for the last sample value recorded
		 * \return The last sample value recorded
		 */
	double getLastSampleValue() const { return lastSampleValue; }

	/**
		 * \brief Accessor for the lastValueRate (Sum of last samples over interval between calculateStatisics calls)
		 * \return The lastValueRate (Sum of last samples over interval between calculateStatisics calls)
		 */
	double getLastValueRate() const { return lastValueRate; }

	/**
		 * \brief Access the enable flag
		 * \return The current value of the enable flag
		 */
	bool isEnabled() const { return enabled; }
};

/**
	 * \brief This class keeps track of statistics for a set of sample values
	 * and provides timing information on the samples.
	 */
class MonitoredQuantity : MonitoredQuantityStats
{
public:
	/**
		 * \brief Instantiates a MonitoredQuantity object
		 * \param expectedCalculationInterval How often calculateStatistics is expected to be called
		 * \param timeWindowForRecentResults Defines the meaning of DataSetType::RECENT
		 */
	explicit MonitoredQuantity(
	    DURATION_T expectedCalculationInterval,
	    DURATION_T timeWindowForRecentResults);

	/**
		 * \brief Adds the specified doubled valued sample value to the monitor instance.
		 * \param value The sample value to add
		 */
	void addSample(const double value = 1.0);

	/**
		 * \brief Adds the specified integer valued sample value to the monitor instance.
		 * \param value The sample value to add
		 */
	void addSample(const int value = 1);

	/**
		 * \brief Adds the specified uint32_t valued sample value to the monitor instance.
		 * \param value The sample value to add
		 */
	void addSample(const uint32_t value = 1);

	/**
		 * \brief Adds the specified uint64_t valued sample value to the monitor instance.
		 * \param value The sample value to add
		 */
	void addSample(const uint64_t value = 1);

	/**
		 * \brief Forces a calculation of the statistics for the monitored quantity.
		 * \param currentTime Time point to use for calculating statistics (if synchronized at a higher level)
		 * \return Whether the statisics were calculated
		 *
		 * Forces a calculation of the statistics for the monitored quantity.
		 * The frequency of the updates to the statistics is driven by how
		 * often this method is called.  It is expected that this method
		 * will be called once per interval specified by
		 * expectedCalculationInterval
		 */
	bool calculateStatistics(TIME_POINT_T currentTime =
	                             getCurrentTime());

	/**
		 * Resets the monitor (zeroes out all counters and restarts the time interval).
		 */
	void reset();

	/**
		 * Enables the monitor (and resets the statistics to provide a fresh start).
		 */
	void enable();

	/**
		 * Disables the monitor.
		 */
	void disable();

	/**
		 * \brief Tests whether the monitor is currently enabled.
		 * \return Whether the monitor is currently enabled.
		 */
	bool isEnabled() const { return enabled; }

	/**
		 * \brief Specifies a new time interval to be used when calculating "recent" statistics.
		 * \param interval The new time interval for calculating "recent" statistics.
		 */
	void setNewTimeWindowForRecentResults(DURATION_T interval);

	/**
		 * \brief Returns the length of the time window that has been specified
		 * for recent results.
		 * \return The length of the time windows for recent results
		 *
		 * Returns the length of the time window that has been specified
		 * for recent results.  (This may be different than the actual
		 * length of the recent time window which is affected by the
		 * interval of calls to the calculateStatistics() method.  Use
		 * a getDuration(RECENT) call to determine the actual recent
		 * time window.)
		 */
	DURATION_T getTimeWindowForRecentResults() const
	{
		return _intervalForRecentStats;
	}

	/**
		 * \brief Returns the expected interval between calculateStatistics calls
		 * \return The expected interval between calculateStatistics calls
		 */
	DURATION_T ExpectedCalculationInterval() const
	{
		return _expectedCalculationInterval;
	}

	/**
		 * \brief Blocks while the MonitoredQuantity is flushed, up to timeout duration.
		 * \param timeout How long to wait for the MonitoredQuantity to be emptied, in seconds
		 * \return Whether the MonitoredQuantity was emptied within the specified timeout
		 */
	bool waitUntilAccumulatorsHaveBeenFlushed(DURATION_T timeout) const;

	/**
		 * \brief Write all our collected statistics into the given Stats struct.
		 * \param stats Destination for copy of collected statistics
		 */
	void getStats(MonitoredQuantityStats& stats) const;

	/**
		 * \brief Returns the current point in time.
		 * \return The current point in time.
		 *
		 *  Returns the current point in time. A negative value indicates
		 *  that an error occurred when fetching the time from the operating
		 *  system.
		 */
	static TIME_POINT_T getCurrentTime();

	// accessors for particular statistics values (more efficient when
	// only a single value is needed)
	TIME_POINT_T getLastCalculationTime() const;  ///< Access the last calculation time
	DURATION_T getFullDuration() const;           ///< Access the full duration of the statistics
	double getRecentValueSum() const;             ///< Access the sum of the value samples in the "recent" time span
	double getRecentValueAverage() const;         ///< Access the average of the value samples in the "recent" time span
	long long getFullSampleCount() const;         ///< Access the count of samples for the entire history of the MonitoredQuantity

private:
	// Prevent copying of the MonitoredQuantity
	MonitoredQuantity(MonitoredQuantity const&) = delete;

	MonitoredQuantity& operator=(MonitoredQuantity const&) = delete;

	// Helper functions.
	void _reset_accumulators();

	void _reset_results();

	std::atomic<TIME_POINT_T> _lastCalculationTime;
	long long _workingSampleCount;
	double _workingValueSum;
	double _workingValueSumOfSquares;
	double _workingValueMin;
	double _workingValueMax;
	double _workingLastSampleValue;

	mutable boost::mutex _accumulationMutex;

	unsigned int _binCount;
	unsigned int _workingBinId;
	std::vector<double> _binValueSumOfSquares;
	std::vector<double> _binValueMin;
	std::vector<double> _binValueMax;

	mutable boost::mutex _resultsMutex;

	DURATION_T _intervalForRecentStats;             // seconds
	const DURATION_T _expectedCalculationInterval;  // seconds
};
}  // namespace artdaq

#endif /* artdaq_core_Core_MonitoredQuantity_hh */
