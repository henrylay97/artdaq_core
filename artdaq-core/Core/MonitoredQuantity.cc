#include "artdaq-core/Core/MonitoredQuantity.hh"

#include <algorithm>
#include <cmath>

using namespace artdaq;

MonitoredQuantity::MonitoredQuantity(
    DURATION_T expectedCalculationInterval,
    DURATION_T timeWindowForRecentResults)
    : _expectedCalculationInterval(expectedCalculationInterval)
{
	setNewTimeWindowForRecentResults(timeWindowForRecentResults);
	enabled = true;
}

void MonitoredQuantity::addSample(const double value)
{
	if (!enabled) { return; }
	boost::mutex::scoped_lock sl(_accumulationMutex);
	if (_lastCalculationTime <= 0.0)
	{
		_lastCalculationTime = getCurrentTime();
	}
	++_workingSampleCount;
	_workingValueSum += value;
	_workingValueSumOfSquares += (value * value);
	if (value < _workingValueMin) { _workingValueMin = value; }
	if (value > _workingValueMax) { _workingValueMax = value; }
	_workingLastSampleValue = value;
}

void MonitoredQuantity::addSample(const int value)
{
	addSample(static_cast<double>(value));
}

void MonitoredQuantity::addSample(const uint32_t value)
{
	addSample(static_cast<double>(value));
}

void MonitoredQuantity::addSample(const uint64_t value)
{
	addSample(static_cast<double>(value));
}

bool MonitoredQuantity::calculateStatistics(TIME_POINT_T currentTime)
{
	if (!enabled) { return false; }
	if (_lastCalculationTime <= 0.0) { return false; }
	if (currentTime - _lastCalculationTime < _expectedCalculationInterval)
	{
		return false;
	}
	// create local copies of the working values to minimize the
	// time that we could block a thread trying to add a sample.
	// Also, reset the working values.
	size_t latestSampleCount;
	double latestValueSum;
	double latestValueSumOfSquares;
	double latestValueMin;
	double latestValueMax;
	DURATION_T latestDuration;
	double latestLastLatchedSampleValue;
	{
		boost::mutex::scoped_lock sl(_accumulationMutex);
		latestSampleCount = _workingSampleCount;
		latestValueSum = _workingValueSum;
		latestValueSumOfSquares = _workingValueSumOfSquares;
		latestValueMin = _workingValueMin;
		latestValueMax = _workingValueMax;
		latestDuration = currentTime - _lastCalculationTime;
		latestLastLatchedSampleValue = _workingLastSampleValue;
		_lastCalculationTime = currentTime;
		_workingSampleCount = 0;
		_workingValueSum = 0.0;
		_workingValueSumOfSquares = 0.0;
		_workingValueMin = INFINITY;
		_workingValueMax = -INFINITY;
	}
	// lock out any interaction with the results while we update them
	{
		boost::mutex::scoped_lock sl(_resultsMutex);
		lastSampleValue = latestLastLatchedSampleValue;
		lastCalculationTime = _lastCalculationTime;
		// we simply add the latest results to the full set
		fullSampleCount += latestSampleCount;
		fullValueSum += latestValueSum;
		fullValueSumOfSquares += latestValueSumOfSquares;
		if (latestValueMin < fullValueMin) { fullValueMin = latestValueMin; }
		if (latestValueMax > fullValueMax) { fullValueMax = latestValueMax; }
		fullDuration += latestDuration;
		// for the recent results, we need to replace the contents of
		// the working bin and re-calculate the recent values
		recentBinnedSampleCounts[_workingBinId] = latestSampleCount;
		recentBinnedValueSums[_workingBinId] = latestValueSum;
		_binValueSumOfSquares[_workingBinId] = latestValueSumOfSquares;
		_binValueMin[_workingBinId] = latestValueMin;
		_binValueMax[_workingBinId] = latestValueMax;
		recentBinnedDurations[_workingBinId] = latestDuration;
		recentBinnedEndTimes[_workingBinId] = _lastCalculationTime;
		if (latestDuration > 0.0)
		{
			lastValueRate = latestValueSum / latestDuration;
		}
		else
		{
			lastValueRate = 0.0;
		}
		recentSampleCount = 0;
		recentValueSum = 0.0;
		recentValueSumOfSquares = 0.0;
		recentValueMin = INFINITY;
		recentValueMax = -INFINITY;
		recentDuration = 0.0;
		for (unsigned int idx = 0; idx < _binCount; ++idx)
		{
			recentSampleCount += recentBinnedSampleCounts[idx];
			recentValueSum += recentBinnedValueSums[idx];
			recentValueSumOfSquares += _binValueSumOfSquares[idx];
			if (_binValueMin[idx] < recentValueMin)
			{
				recentValueMin = _binValueMin[idx];
			}
			if (_binValueMax[idx] > recentValueMax)
			{
				recentValueMax = _binValueMax[idx];
			}
			recentDuration += recentBinnedDurations[idx];
		}
		// update the working bin ID here so that we are ready for
		// the next calculation request
		++_workingBinId;
		if (_workingBinId >= _binCount) { _workingBinId = 0; }
		// calculate the derived full values
		if (fullDuration > 0.0)
		{
			fullSampleRate = static_cast<double>(fullSampleCount) / fullDuration;
			fullValueRate = static_cast<double>(fullValueSum) / fullDuration;
		}
		else
		{
			fullSampleRate = 0.0;
			fullValueRate = 0.0;
		}
		if (fullSampleCount > 0)
		{
			fullValueAverage = fullValueSum / static_cast<double>(fullSampleCount);
			double squareAvg = fullValueSumOfSquares / static_cast<double>(fullSampleCount);
			double avg = fullValueSum / static_cast<double>(fullSampleCount);
			double sigSquared = squareAvg - avg * avg;
			if (sigSquared > 0.0)
			{
				fullValueRMS = sqrt(sigSquared);
			}
			else
			{
				fullValueRMS = 0.0;
			}
		}
		else
		{
			fullValueAverage = 0.0;
			fullValueRMS = 0.0;
		}
		// calculate the derived recent values
		if (recentDuration > 0.0)
		{
			recentSampleRate = static_cast<double>(recentSampleCount) / recentDuration;
			recentValueRate = static_cast<double>(recentValueSum) / recentDuration;
		}
		else
		{
			recentSampleRate = 0.0;
			recentValueRate = 0.0;
		}
		if (recentSampleCount > 0)
		{
			recentValueAverage = recentValueSum / static_cast<double>(recentSampleCount);
			double squareAvg = recentValueSumOfSquares /
			                   static_cast<double>(recentSampleCount);
			double avg = recentValueSum / static_cast<double>(recentSampleCount);
			double sigSquared = squareAvg - avg * avg;
			if (sigSquared > 0.0)
			{
				recentValueRMS = sqrt(sigSquared);
			}
			else
			{
				recentValueRMS = 0.0;
			}
		}
		else
		{
			recentValueAverage = 0.0;
			recentValueRMS = 0.0;
		}
	}
	return true;
}

void MonitoredQuantity::_reset_accumulators()
{
	_lastCalculationTime = 0;
	_workingSampleCount = 0;
	_workingValueSum = 0.0;
	_workingValueSumOfSquares = 0.0;
	_workingValueMin = INFINITY;
	_workingValueMax = -INFINITY;
	_workingLastSampleValue = 0;
}

void MonitoredQuantity::_reset_results()
{
	_workingBinId = 0;
	for (unsigned int idx = 0; idx < _binCount; ++idx)
	{
		recentBinnedSampleCounts[idx] = 0;
		recentBinnedValueSums[idx] = 0.0;
		_binValueSumOfSquares[idx] = 0.0;
		_binValueMin[idx] = INFINITY;
		_binValueMax[idx] = -INFINITY;
		recentBinnedDurations[idx] = 0.0;
		recentBinnedEndTimes[idx] = 0.0;
	}
	fullSampleCount = 0;
	fullSampleRate = 0.0;
	fullValueSum = 0.0;
	fullValueSumOfSquares = 0.0;
	fullValueAverage = 0.0;
	fullValueRMS = 0.0;
	fullValueMin = INFINITY;
	fullValueMax = -INFINITY;
	fullValueRate = 0.0;
	fullDuration = 0.0;
	recentSampleCount = 0;
	recentSampleRate = 0.0;
	recentValueSum = 0.0;
	recentValueSumOfSquares = 0.0;
	recentValueAverage = 0.0;
	recentValueRMS = 0.0;
	recentValueMin = INFINITY;
	recentValueMax = -INFINITY;
	recentValueRate = 0.0;
	recentDuration = 0.0;
	lastSampleValue = 0.0;
	lastValueRate = 0.0;
}

void MonitoredQuantity::reset()
{
	{
		boost::mutex::scoped_lock sl(_accumulationMutex);
		_reset_accumulators();
	}
	{
		boost::mutex::scoped_lock sl(_resultsMutex);
		_reset_results();
	}
}

void MonitoredQuantity::enable()
{
	if (!enabled)
	{
		reset();
		enabled = true;
	}
}

void MonitoredQuantity::disable()
{
	// It is faster to just set _enabled to false than to test and set
	// it conditionally.
	enabled = false;
}

void MonitoredQuantity::setNewTimeWindowForRecentResults(DURATION_T interval)
{
	// lock the results objects since we're dramatically changing the
	// bins used for the recent results
	{
		boost::mutex::scoped_lock sl(_resultsMutex);
		_intervalForRecentStats = interval;
		// determine how many bins we should use in our sliding window
		// by dividing the input time window by the expected calculation
		// interval and rounding to the nearest integer.
		// In case that the calculation interval is larger then the
		// interval for recent stats, keep the last one.
		_binCount = std::max(1U, static_cast<unsigned int>(std::lround(_intervalForRecentStats / _expectedCalculationInterval)));
		// create the vectors for the binned quantities
		recentBinnedSampleCounts.reserve(_binCount);
		recentBinnedValueSums.reserve(_binCount);
		_binValueSumOfSquares.reserve(_binCount);
		_binValueMin.reserve(_binCount);
		_binValueMax.reserve(_binCount);
		recentBinnedDurations.reserve(_binCount);
		recentBinnedEndTimes.reserve(_binCount);
		_reset_results();
	}
	{
		boost::mutex::scoped_lock sl(_accumulationMutex);
		_reset_accumulators();
	}
	// call the reset method to populate the correct initial values
	// for the internal sample data
	//reset();
}

bool MonitoredQuantity::
    waitUntilAccumulatorsHaveBeenFlushed(DURATION_T timeout) const
{
	timeout /= 10;
	{
		boost::mutex::scoped_lock sl(_accumulationMutex);
		if (_workingSampleCount == 0) { return true; }
	}
	auto sleepTime = static_cast<int64_t>(timeout * 100000.0);
	for (auto idx = 0; idx < 10; ++idx)
	{
		usleep(sleepTime);
		{
			boost::mutex::scoped_lock sl(_accumulationMutex);
			if (_workingSampleCount == 0) { return true; }
		}
	}
	return false;
}

void MonitoredQuantity::getStats(MonitoredQuantityStats& s) const
{
	boost::mutex::scoped_lock results(_resultsMutex);
	s.fullSampleCount = fullSampleCount;
	s.fullSampleRate = fullSampleRate;
	s.fullValueSum = fullValueSum;
	s.fullValueSumOfSquares = fullValueSumOfSquares;
	s.fullValueAverage = fullValueAverage;
	s.fullValueRMS = fullValueRMS;
	s.fullValueMin = fullValueMin;
	s.fullValueMax = fullValueMax;
	s.fullValueRate = fullValueRate;
	s.fullDuration = fullDuration;
	s.recentSampleCount = recentSampleCount;
	s.recentSampleRate = recentSampleRate;
	s.recentValueSum = recentValueSum;
	s.recentValueSumOfSquares = recentValueSumOfSquares;
	s.recentValueAverage = recentValueAverage;
	s.recentValueRMS = recentValueRMS;
	s.recentValueMin = recentValueMin;
	s.recentValueMax = recentValueMax;
	s.recentValueRate = recentValueRate;
	s.recentDuration = recentDuration;
	s.recentBinnedSampleCounts.resize(_binCount);
	s.recentBinnedValueSums.resize(_binCount);
	s.recentBinnedDurations.resize(_binCount);
	s.recentBinnedEndTimes.resize(_binCount);
	unsigned int sourceBinId = _workingBinId;
	for (unsigned int idx = 0; idx < _binCount; ++idx)
	{
		if (sourceBinId >= _binCount) { sourceBinId = 0; }
		s.recentBinnedSampleCounts[idx] = recentBinnedSampleCounts[sourceBinId];
		s.recentBinnedValueSums[idx] = recentBinnedValueSums[sourceBinId];
		s.recentBinnedDurations[idx] = recentBinnedDurations[sourceBinId];
		s.recentBinnedEndTimes[idx] = recentBinnedEndTimes[sourceBinId];
		++sourceBinId;
	}
	s.lastSampleValue = lastSampleValue;
	s.lastValueRate = lastValueRate;
	s.lastCalculationTime = lastCalculationTime;
	s.enabled = enabled;
}

MonitoredQuantity::TIME_POINT_T MonitoredQuantity::getCurrentTime()
{
	TIME_POINT_T result = -1.0;
	timeval now;
	if (gettimeofday(&now, nullptr) == 0)
	{
		result = static_cast<TIME_POINT_T>(now.tv_sec);
		result += static_cast<TIME_POINT_T>(now.tv_usec) / (1000 * 1000);
	}
	return result;
}

MonitoredQuantity::TIME_POINT_T MonitoredQuantity::getLastCalculationTime() const
{
	boost::mutex::scoped_lock results(_resultsMutex);
	return lastCalculationTime;
}

MonitoredQuantity::DURATION_T MonitoredQuantity::getFullDuration() const
{
	boost::mutex::scoped_lock results(_resultsMutex);
	return fullDuration;
}

double MonitoredQuantity::getRecentValueSum() const
{
	boost::mutex::scoped_lock results(_resultsMutex);
	return recentValueSum;
}

double MonitoredQuantity::getRecentValueAverage() const
{
	boost::mutex::scoped_lock results(_resultsMutex);
	return recentValueAverage;
}

size_t MonitoredQuantity::getFullSampleCount() const
{
	boost::mutex::scoped_lock results(_resultsMutex);
	return fullSampleCount;
}
