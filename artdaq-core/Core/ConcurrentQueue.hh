#ifndef artdaq_core_Core_ConcurrentQueue_hh
#define artdaq_core_Core_ConcurrentQueue_hh

#include <algorithm>
#include <cstddef>
#include <exception>
#include <limits>
#include <list>

#include <iostream>   // debugging
#include "tracemf.h"  // TRACE - note: no #define TRACE_NAME in .hh files

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <type_traits>

// #include <boost/date_time/posix_time/posix_time_types.hpp>
// #include <boost/utility/enable_if.hpp>
// #include <boost/thread/condition.hpp>
// #include <boost/thread/mutex.hpp>
// #include <boost/thread/xtime.hpp>

namespace artdaq {
/**
	   Class template ConcurrentQueue provides a FIFO that can be used
	   to communicate data between multiple producer and consumer
	   threads in an application.

	   The template policy EnqPolicy determines the behavior of the
	   enqNowait function. In all cases, this function will return
	   promptly (that is, it will not wait for a full queue to become
	   not-full). However, what is done in the case of the queue being
	   full depends on the policy chosen:

	   FailIfFull: a std::exeption is thrown if the queue is full.

	   KeepNewest: the head of the FIFO is popped (and destroyed),
	   and the new item is added to the FIFO. The function returns
	   the number of popped (dropped) element.

	   RejectNewest: the new item is not put onto the FIFO.
	   The function returns the dropped event count (1) if the
	   item cannot be added.

	*/

namespace detail {
/**
		 * We shall use artdaq::detail::seconds as our "standard" duration
		 * type. Note that this differs from std::chrono::seconds, which has
		 * a representation in some integer type of at least 35 bits.
		 *
		 * daqrate::duration dur(1.0) represents a duration of 1 second.
		 * daqrate::duration dur2(0.001) represents a duration of 1
		 * millisecond.
		*/
typedef std::chrono::duration<double> seconds;

typedef size_t MemoryType;  ///< Basic unit of data storage and pointer types

/**
		  This template is using SFINAE to figure out if the class used to
		  instantiate the ConcurrentQueue template has a method memoryUsed
		  returning the number of bytes occupied by the class itself.
		*/
template<typename T>
class hasMemoryUsed
{
	typedef char TrueType;

	struct FalseType
	{
		TrueType _[2];
	};

	template<MemoryType (T::*)() const>
	struct TestConst;

	template<typename C>
	static TrueType test(TestConst<&C::memoryUsed>*)
	{
		return 0;
	}

	template<typename C>
	static FalseType test(...)
	{
		return {};
	}

public:
	/**
			 * \brief Use SFINAE to figure out if the class used to
		  * instantiate the ConcurrentQueue template has a method memoryUsed
		  * returning the number of bytes occupied by the class itself.
			 */
	static const bool value = (sizeof(test<T>(nullptr)) == sizeof(TrueType));  // NOLINT(cert-err58-cpp)
};

/**
		 * \brief Returns the memory used by an object
		 * \tparam T The type of the object
		 * \param t A pair of object and size_t
		 * \return If the object has a memoryUsed function, the result of that function. Otherwise 0.
		 */
template<typename T>
MemoryType
memoryUsage(const std::pair<T, size_t>& t)
{
	MemoryType usage(0UL);
	try
	{
		usage = t.first.memoryUsed();
	}
	catch (...)
	{}
	return usage;
}

/**
		 * \brief Returns the memory used by an object. Uses hasMemoryUsed to determine if there is a memoryUsed function in object.
		 * \tparam T Type of object t
		 * \param t Object to retrieve memory usage from
		 * \return Self-reported memory usage of object
		 */
template<typename T>
typename std::enable_if<hasMemoryUsed<T>::value, MemoryType>::type
memoryUsage(const T& t)
{
	MemoryType usage(0UL);
	try
	{
		usage = t.memoryUsed();
	}
	catch (...)
	{}
	return usage;
}

/**
		 * \brief Returns the memory used by an object, as obtained through sizeof
		 * \tparam T Type of object t
		 * \param t Object to retrieve memory usage from
		 * \return sizeof(t)
		 */
template<typename T>
typename std::enable_if<!hasMemoryUsed<T>::value, MemoryType>::type
memoryUsage(const T& t)
{
	return sizeof(t);
}
}  // end namespace detail

/**
	 * \brief ConcurrentQueue policy to throw an exception when the queue is full
	 * \tparam T Type of element to store in queue
	 */
template<class T>
struct FailIfFull
{
	typedef bool ReturnType;  ///< Type returned by doEnq

	typedef T ValueType;                                ///< Type of values stored in queue
	typedef std::list<T> SequenceType;                  ///< Type of seqeuences of values
	typedef typename SequenceType::size_type SizeType;  ///< Size type of seqeuences

	/**
		 * \brief Exception thrown by FailIfFull policy when an enqueue operation is attempted on a full queue
		 */
	struct QueueIsFull : public std::exception
	{
		/**
			 * \brief Describe exception
			 * \return Description of QueueIsFull exception
			 */
		virtual const char* what() const throw()
		{
			return "Cannot add item to a full queue";
		}
	};

	/**
		 * \brief Inserts element into the ConcurrentQueue
		 * \param item Item to insert
		 * \param elements The ConcurrentQueue data
		 * \param size Number of elements in the ConcurrentQueue
		 * \param itemSize Size of the newly-inserted element
		 * \param used Memory used by the ConcurrentQueue elements
		 * \param nonempty Condition variable to notify readers of new data on queue
		 */
	static void doInsert(
	    T const& item,
	    SequenceType& elements,
	    SizeType& size,
	    detail::MemoryType const& itemSize,
	    detail::MemoryType& used,
	    std::condition_variable& nonempty)
	{
		elements.push_back(item);
		++size;
		used += itemSize;
		nonempty.notify_one();
	}

	/**
		 * \brief Attempts to enqueue an item
		 * \param item Item to enqueue
		 * \param elements The ConcurrentQueue data
		 * \param size Number of elements in the ConcurrentQueue
		 * \param capacity Maximum number of elements in the ConcurrentQueue
		 * \param used Memory used by the ConcurrentQueue elements
		 * \param memory Amount of memory available for use by the ConcurrentQueue
		 * \param elementsDropped Number of failed insert operations
		 * \param nonempty Condition variable to notify readers of new data on the queue
		 * \return Whether the enqueue operation succeeded
		 * \exception queueIsFull if the queue is full
		 */
	static ReturnType doEnq(
	    T const& item,
	    SequenceType& elements,
	    SizeType& size,
	    SizeType& capacity,
	    detail::MemoryType& used,
	    detail::MemoryType& memory,
	    size_t& elementsDropped,
	    std::condition_variable& nonempty)
	{
		detail::MemoryType itemSize = detail::memoryUsage(item);
		if (size >= capacity || used + itemSize > memory)
		{
			++elementsDropped;
			throw QueueIsFull();
		}
		else
		{
			doInsert(item, elements, size, itemSize, used, nonempty);
		}
		return true;
	}
};

/**
	* \brief ConcurrentQueue policy to discard oldest elements when the queue is full
	* \tparam T Type of element to store in queue
	*/
template<class T>
struct KeepNewest
{
	typedef std::pair<T, size_t> ValueType;             ///< Type of elements stored in the queue
	typedef std::list<T> SequenceType;                  ///< Type of sequences of items
	typedef typename SequenceType::size_type SizeType;  ///< Size type of seqeuences
	typedef SizeType ReturnType;                        ///< Type returned by doEnq

	/**
		* \brief Inserts element into the ConcurrentQueue
		* \param item Item to insert
		* \param elements The ConcurrentQueue data
		* \param size Number of elements in the ConcurrentQueue
		* \param itemSize Size of the newly-inserted element
		* \param used Memory used by the ConcurrentQueue elements
		* \param nonempty Condition variable to notify readers of new data on queue
		*/
	static void doInsert(
	    T const& item,
	    SequenceType& elements,
	    SizeType& size,
	    detail::MemoryType const& itemSize,
	    detail::MemoryType& used,
	    std::condition_variable& nonempty)
	{
		elements.push_back(item);
		++size;
		used += itemSize;
		nonempty.notify_one();
	}

	/**
		* \brief Attempts to enqueue an item
		* \param item Item to enqueue
		* \param elements The ConcurrentQueue data
		* \param size Number of elements in the ConcurrentQueue
		* \param capacity Maximum number of elements in the ConcurrentQueue
		* \param used Memory used by the ConcurrentQueue elements
		* \param memory Amount of memory available for use by the ConcurrentQueue
		* \param elementsDropped Number of failed insert operations
		* \param nonempty Condition variable to notify readers of new data on the queue
		* \return Number of elements removed from the queue
		*/
	static ReturnType doEnq(
	    T const& item,
	    SequenceType& elements,
	    SizeType& size,
	    SizeType& capacity,
	    detail::MemoryType& used,
	    detail::MemoryType& memory,
	    size_t& elementsDropped,
	    std::condition_variable& nonempty)
	{
		SizeType elementsRemoved(0);
		detail::MemoryType itemSize = detail::memoryUsage(item);
		while ((size == capacity || used + itemSize > memory) && !elements.empty())
		{
			SequenceType holder;
			// Move the item out of elements in a manner that will not throw.
			holder.splice(holder.begin(), elements, elements.begin());
			// Record the change in the length of elements.
			--size;
			used -= detail::memoryUsage(holder.front());
			++elementsRemoved;
		}
		if (size < capacity && used + itemSize <= memory)
		// we succeeded to make enough room for the new element
		{
			doInsert(item, elements, size, itemSize, used, nonempty);
		}
		else
		{
			// we cannot add the new element
			++elementsRemoved;
		}
		elementsDropped += elementsRemoved;
		return elementsRemoved;
	}
};

/**
	* \brief ConcurrentQueue policy to discard new elements when the queue is full
	* \tparam T Type of element to store in queue
	*/
template<class T>
struct RejectNewest
{
	typedef std::pair<T, size_t> ValueType;             ///< Type of elements stored in the queue
	typedef std::list<T> SequenceType;                  ///< Type of sequences of items
	typedef typename SequenceType::size_type SizeType;  ///< Size type of seqeuences
	typedef SizeType ReturnType;                        ///< Type returned by doEnq

	/**
		* \brief Inserts element into the ConcurrentQueue
		* \param item Item to insert
		* \param elements The ConcurrentQueue data
		* \param size Number of elements in the ConcurrentQueue
		* \param itemSize Size of the newly-inserted element
		* \param used Memory used by the ConcurrentQueue elements
		* \param nonempty Condition variable to notify readers of new data on queue
		*/
	static void doInsert(
	    T const& item,
	    SequenceType& elements,
	    SizeType& size,
	    detail::MemoryType const& itemSize,
	    detail::MemoryType& used,
	    std::condition_variable& nonempty)
	{
		elements.push_back(item);
		++size;
		used += itemSize;
		nonempty.notify_one();
	}

	/**
		* \brief Attempts to enqueue an item
		* \param item Item to enqueue
		* \param elements The ConcurrentQueue data
		* \param size Number of elements in the ConcurrentQueue
		* \param capacity Maximum number of elements in the ConcurrentQueue
		* \param used Memory used by the ConcurrentQueue elements
		* \param memory Amount of memory available for use by the ConcurrentQueue
		* \param elementsDropped Number of failed insert operations
		* \param nonempty Condition variable to notify readers of new data on the queue
		* \return Number of elements removed from the queue (1 if new element was rejected, 0 otherwise)
		*/
	static ReturnType doEnq(
	    T const& item,
	    SequenceType& elements,
	    SizeType& size,
	    SizeType& capacity,
	    detail::MemoryType& used,
	    detail::MemoryType& memory,
	    size_t& elementsDropped,
	    std::condition_variable& nonempty)
	{
		detail::MemoryType itemSize = detail::memoryUsage(item);
		if (size < capacity && used + itemSize <= memory)
		{
			doInsert(item, elements, size, itemSize, used, nonempty);
			return 0;
		}
		++elementsDropped;
		return 1;
	}
};

/**
	   ConcurrentQueue<T> class template declaration.
	*/
template<class T, class EnqPolicy = FailIfFull<T>>
class ConcurrentQueue
{
public:
	typedef typename EnqPolicy::ValueType ValueType;        ///< Type of values stored in ConcurrentQueue
	typedef typename EnqPolicy::SequenceType SequenceType;  ///< Type of sequence used by ConcurrentQueue
	typedef typename SequenceType::size_type SizeType;      ///< Type for indexes in sequence

	/**
		\brief   ConcurrentQueue is always bounded. By default, the bound is
		   absurdly large.
		\param maxSize Maximum size of the ConcurrentQueue (default: SizeType::max)
		\param maxMemory Maximum memory size of the ConcurrentQueue (default: MemoryType::max)
		*/
	explicit ConcurrentQueue(
	    SizeType maxSize = std::numeric_limits<SizeType>::max(),
	    detail::MemoryType maxMemory = std::numeric_limits<detail::MemoryType>::max());

	/**
		   Applications should arrange to make sure that the destructor of
		   a ConcurrentQueue is not called while some other thread is
		   using that queue. There is some protection against doing this,
		   but it seems impossible to make sufficient protection.
		*/
	~ConcurrentQueue();

	/**
		   Copying a ConcurrentQueue is illegal, as is asigning to a
		   ConcurrentQueue. The copy constructor and copy assignment
		   operator are both private and deleted.
		*/

	/**
		 * \brief Add a copy if item to the queue, according to the rules determined by the EnqPolicy.
		 * \param item Item to enqueue
		 * \return EnqPolicy::ReturnType result of enqueue operation
		 *
		 *  Add a copy if item to the queue, according to the rules
		 *  determined by the EnqPolicy; see documentation above the the
		 *  provided EnqPolicy choices.  This may throw any exception
		 *  thrown by the assignment operator of type T, or badAlloc.
		 */
	typename EnqPolicy::ReturnType enqNowait(T const& item);

	/**
		 * \brief  Add a copy of item to the queue.
		 * \param item Item to enqueue
		 *
		 * Add a copy of item to the queue. If the queue is full wait
		 * until it becomes non-full. This may throw any exception thrown
		 * by the assignment operator of type T, or badAlloc.
		*/
	void enqWait(T const& item);

	/**
		 * \brief Add a copy of item to the queue, waiting for the queue to be non-full.
		 * \param item Item to enqueue
		 * \param wait Maximum time (in seconds) to wait for queue to be non-full
		 * \return Whether the item was added before the timeout expired
		 *
		 * Add a copy of item to the queue. If the queue is full wait
		 * until it becomes non-full or until timeDuration has passed.
		 * Return true if the items has been put onto the queue or
		 * false if the timeout has expired. This may throw any exception
		 * thrown by the assignment operator of T, or badAlloc.
		 */
	bool enqTimedWait(T const& item, detail::seconds const& wait);

	/**
		 * \brief Assign the value at the head of the queue to item and then
		 * remove the head of the queue.
		 * \param item Reference to output item
		 * \return If the dequeue operation was successful
		 *
		 * Assign the value at the head of the queue to item and then
		 * remove the head of the queue. If successful, return true; on
		 * failure, return false. This function fill fail without waiting
		 * if the queue is empty. This function may throw any exception
		 * thrown by the assignment operator of type EnqPolicy::ValueType.
		 */
	bool deqNowait(ValueType& item);

	/**
		 * \brief Assign the value of the head of the queue to item and then
		 * remove the head of the queue.
		 * \param item Reference to output item
		 *
		 * Assign the value of the head of the queue to item and then
		 * remove the head of the queue. If the queue is empty wait until
		 * is has become non-empty. This may throw any exception thrown by
		 * the assignment operator of type EnqPolicy::ValueType.
		 */
	void deqWait(ValueType& item);

	/**
		 * \brief Assign the value at the head of the queue to item and then remove the head of the queue.
		 * \param item Reference to output item
		 * \param wait Maximum number of seconds to wait for dequeue
		 * \return Whether an item was dequeued in the given interval
		 *
		 * Assign the value at the head of the queue to item and then
		 * remove the head of the queue. If the queue is empty wait until
		 * is has become non-empty or until timeDuration has passed.
		 * Return true if an item has been removed from the queue
		 * or false if the timeout has expired. This may throw any
		 * exception thrown by the assignment operator of type EnqPolicy::ValueType.
		 */
	bool deqTimedWait(ValueType& item, detail::seconds const& wait);

	/**
		   Return true if the queue is empty, and false if it is not.
		\return True if the queue is empty
		*/
	bool empty() const;

	/**
		   Return true if the queue is full, and false if it is not.
		\return True if the queue is full
		*/
	bool full() const;

	/**
		   Return the size of the queue, that is, the number of items it
		   contains.
		\return The number of items in the queue
		*/
	SizeType size() const;

	/**
		 *\brief Return the capacity of the queue, that is, the maximum number
		 *  of items it can contain.
		* \return The maxmium number of items the ConcurrentQueue can contain
		*/
	SizeType capacity() const;

	/**
		   Reset the capacity of the queue. This can only be done if the
		   queue is empty. This function returns false if the queue was
		   not modified, and true if it was modified.
		   \param capacity The new capacity of the queue
		\return True if the queue was modified
		*/
	bool setCapacity(SizeType capacity);

	/**
		   \brief Return the memory in bytes used by items in the queue
		\return The amount of memory in use by queue items
		*/
	detail::MemoryType used() const;

	/**
		   \brief Return the memory of the queue in bytes, that is, the maximum memory
		   the items in the queue may occupy
		\return The amount of memory allocated by the queue
		*/
	detail::MemoryType memory() const;

	/**
		  \brief Reset the memory usage in bytes of the queue. A value of 0 disabled the
		   memory check. This can only be done if the
		   queue is empty.
		\param maxMemory Sets the maximum amount of memory used by the queue
		\return This function returns false if the queue was
		   not modified, and true if it was modified.
		*/
	bool setMemory(detail::MemoryType maxMemory);

	/**
		   \brief Remove all items from the queue. This changes the size to zero
		   but does not change the capacity.
		   \return The number of cleared events.
		*/
	SizeType clear();

	/**
		 * \brief Adds the passed count to the counter of dropped events
		 * \param dropped Number of events dropped by code outside ConcurrentQueue
		 */
	void addExternallyDroppedEvents(SizeType dropped);

	/**
		 * \brief Is the reader connected and ready for items to appear on the queue?
		 * \return If the reader is connected and ready for items to be written to the queue.
		 */
	bool queueReaderIsReady() const { return readerReady_; }

	/**
		 * \brief Set the ready flag for the reader
		 * \param rdy Value of the ready flag (default: true)
		 *
		 * Sets the ready flag for the reader, and the time that the reader became ready or unready.
		 * Used to help _artdaq_ wait for _art_ to finish initializing.
		 */
	void setReaderIsReady(bool rdy = true)
	{
		readyTime_ = std::chrono::steady_clock::now();
		readerReady_ = rdy;
	}

	/**
			\brief Gets the time at which the queue became ready
		\return Time at which the queue became ready
		*/
	std::chrono::steady_clock::time_point getReadyTime() const { return readyTime_; }

private:
	typedef std::lock_guard<std::mutex> LockType;
	typedef std::unique_lock<std::mutex> WaitLockType;

	mutable std::mutex protectElements_;
	mutable std::condition_variable queueNotEmpty_;
	mutable std::condition_variable queueNotFull_;

	std::chrono::steady_clock::time_point readyTime_;
	bool readerReady_{false};
	SequenceType elements_;
	SizeType capacity_;
	SizeType size_;
	/*
		  N.B.: we rely on SizeType *not* being some synthesized large
		  type, so that reading the value is an atomic action, as is
		  incrementing or decrementing the value. We do *not* assume that
		  there is any atomic getAndIncrement or getAndDecrement
		  operation.
		*/
	detail::MemoryType memory_;
	detail::MemoryType used_{0};
	size_t elementsDropped_{0};

	/*
		  These private member functions assume that whatever locks
		  necessary for safe operation have already been obtained.
		*/

	/*
		  Insert the given item into the list, if it is not already full,
		  and increment size. Return true if the item is inserted, and
		  false if not.
		*/
	bool insertIfPossible(T const& item);

	/**
		 * \brief Remove the object at the head of the queue, if there is one, and
		 * assign item the value of this object.
		 * \param item Object at the head of the queue, if there is one.
		 * \return Return true if the queue was nonempty,
		 *  and false if the queue was empty.
		 *
		 * Remove the object at the head of the queue, if there is one, and
		 * assign item the value of this object. The assignment may throw an
		 * exception; even if it does, the head will have been removed from
		 * the queue, and the size appropriately adjusted. It is assumed
		 * the queue is nonempty.
		 */
	bool removeHeadIfPossible(ValueType& item);

	/**
		 * \brief Remove the object at the head of the queue, and assign item the
		  value of this object
		 * \param item Object at the head of the queue.
		 *
		 * Remove the object at the head of the queue, and assign item the
		 * value of this object. The assignment may throw an exception;
		 * even if it does, the head will have been removed from the queue,
		 * and the size appropriately adjusted. It is assumed the queue is
		 * nonempty.
		 */
	void removeHead(ValueType& item);

	void assignItem(T& item, const T& element);

	void assignItem(std::pair<T, size_t>& item, const T& element);

	/*
		  Return false if the queue can accept new entries.
		*/
	bool isFull() const;

	/*
		  These functions are declared private and not implemented to
		  prevent their use.
		*/
	ConcurrentQueue(ConcurrentQueue<T, EnqPolicy> const&) = delete;

	ConcurrentQueue& operator=(ConcurrentQueue<T, EnqPolicy> const&) = delete;
	ConcurrentQueue(ConcurrentQueue<T, EnqPolicy>&&) = default;
	ConcurrentQueue& operator=(ConcurrentQueue<T, EnqPolicy>&&) = default;
};

//------------------------------------------------------------------
// Implementation follows
//------------------------------------------------------------------

template<class T, class EnqPolicy>
ConcurrentQueue<T, EnqPolicy>::ConcurrentQueue(
    SizeType maxSize,
    detail::MemoryType maxMemory)
    : protectElements_()
    , readyTime_(std::chrono::steady_clock::now())
    , elements_()
    , capacity_(maxSize)
    , size_(0)
    , memory_(maxMemory)
{}

template<class T, class EnqPolicy>
ConcurrentQueue<T, EnqPolicy>::~ConcurrentQueue()
{
	LockType lock(protectElements_);
	elements_.clear();
	size_ = 0;
	used_ = 0;
	elementsDropped_ = 0;
}

// enqueue methods - 3 - enqNowait, enqWait, enqTimedWait

template<class T, class EnqPolicy>
typename EnqPolicy::ReturnType ConcurrentQueue<T, EnqPolicy>::enqNowait(T const& item)
{
	TLOG(12, "ConcurrentQueue") << "enqNowait enter size=" << size_ << " capacity=" << capacity_ << " used=" << used_ << " memory=" << memory_;
	LockType lock(protectElements_);
	auto retval = EnqPolicy::doEnq(item, elements_, size_, capacity_, used_, memory_,
	                               elementsDropped_, queueNotEmpty_);
	TLOG(12, "ConcurrentQueue") << "enqNowait returning " << retval;
	return retval;
}

template<class T, class EnqPolicy>
void ConcurrentQueue<T, EnqPolicy>::enqWait(T const& item)
{
	TLOG(13, "ConcurrentQueue") << "enqWait enter";
	WaitLockType lock(protectElements_);
	while (isFull()) { queueNotFull_.wait(lock); }
	EnqPolicy::doInsert(item, elements_, size_,
	                    detail::memoryUsage(item), used_, queueNotEmpty_);
	TLOG(13, "ConcurrentQueue") << "enqWait returning";
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::enqTimedWait(T const& item, detail::seconds const& waitTime)
{
	TLOG(14, "ConcurrentQueue") << "ConcurrentQueue<T,EnqPolicy>::enqTimedWait enter with waitTime=" << std::chrono::duration_cast<std::chrono::milliseconds>(waitTime).count() << " ms size=" << size_
	                            << " capacity=" << capacity_ << " used=" << used_ << " memory=" << memory_;
	WaitLockType lock(protectElements_);
	if (isFull())
	{
		queueNotFull_.wait_for(lock, waitTime);
	}
	bool retval = insertIfPossible(item);
	TLOG(14, "ConcurrentQueue") << "ConcurrentQueue<T,EnqPolicy>::enqTimedWait returning " << retval;
	return retval;
}

// dequeue methods - 3 - deqNowait, deqWait, deqTimedWait

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::deqNowait(ValueType& item)
{
	TLOG(15, "ConcurrentQueue") << "ConcurrentQueue<T, EnqPolicy>::deqNowait enter";
	LockType lock(protectElements_);
	bool retval = removeHeadIfPossible(item);
	TLOG(15, "ConcurrentQueue") << "ConcurrentQueue<T, EnqPolicy>::deqNowait returning " << retval;
	return retval;
}

template<class T, class EnqPolicy>
void ConcurrentQueue<T, EnqPolicy>::deqWait(ValueType& item)
{
	TLOG(16, "ConcurrentQueue") << "ConcurrentQueue<T, EnqPolicy>::deqWait enter";
	WaitLockType lock(protectElements_);
	while (size_ == 0) { queueNotEmpty_.wait(lock); }
	removeHead(item);
	TLOG(16, "ConcurrentQueue") << "ConcurrentQueue<T, EnqPolicy>::deqWait returning";
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::deqTimedWait(ValueType& item, detail::seconds const& waitTime)
{
	TLOG(17, "ConcurrentQueue") << "ConcurrentQueue<T, EnqPolicy>::deqTimedWait enter with waitTime=" << std::chrono::duration_cast<std::chrono::milliseconds>(waitTime).count() << " ms size=" << size_;
	WaitLockType lock(protectElements_);
	if (size_ == 0)
	{
		queueNotEmpty_.wait_for(lock, waitTime);
	}
	bool retval = removeHeadIfPossible(item);
	TLOG(17, "ConcurrentQueue") << "ConcurrentQueue<T, EnqPolicy>::deqTimedWait returning " << retval << " size=" << size_;
	return retval;
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::empty() const
{
	// No lock is necessary: the read is atomic.
	return size_ == 0;
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::full() const
{
	LockType lock(protectElements_);
	return isFull();
}

template<class T, class EnqPolicy>
typename ConcurrentQueue<T, EnqPolicy>::SizeType
ConcurrentQueue<T, EnqPolicy>::size() const
{
	// No lock is necessary: the read is atomic.
	return size_;
}

template<class T, class EnqPolicy>
typename ConcurrentQueue<T, EnqPolicy>::SizeType
ConcurrentQueue<T, EnqPolicy>::capacity() const
{
	// No lock is necessary: the read is atomic.
	return capacity_;
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::setCapacity(SizeType newcapacity)
{
	LockType lock(protectElements_);
	bool isEmpty = (size_ == 0);
	if (isEmpty) { capacity_ = newcapacity; }
	return isEmpty;
}

template<class T, class EnqPolicy>
detail::MemoryType
ConcurrentQueue<T, EnqPolicy>::used() const
{
	// No lock is necessary: the read is atomic.
	return used_;
}

template<class T, class EnqPolicy>
detail::MemoryType
ConcurrentQueue<T, EnqPolicy>::memory() const
{
	// No lock is necessary: the read is atomic.
	return memory_;
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::setMemory(detail::MemoryType maxMemory)
{
	LockType lock(protectElements_);
	bool isEmpty = (size_ == 0);
	if (isEmpty) { memory_ = maxMemory; }
	return isEmpty;
}

template<class T, class EnqPolicy>
typename ConcurrentQueue<T, EnqPolicy>::SizeType
ConcurrentQueue<T, EnqPolicy>::clear()
{
	LockType lock(protectElements_);
	SizeType clearedEvents = size_;
	elementsDropped_ += size_;
	elements_.clear();
	size_ = 0;
	used_ = 0;
	return clearedEvents;
}

template<class T, class EnqPolicy>
void ConcurrentQueue<T, EnqPolicy>::addExternallyDroppedEvents(SizeType dropped)
{
	LockType lock(protectElements_);
	elementsDropped_ += dropped;
}

//-----------------------------------------------------------
// Private member functions
//-----------------------------------------------------------

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::insertIfPossible(T const& item)
{
	if (isFull())
	{
		++elementsDropped_;
		return false;
	}
	else
	{
		EnqPolicy::doInsert(item, elements_, size_,
		                    detail::memoryUsage(item), used_, queueNotEmpty_);
		return true;
	}
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::removeHeadIfPossible(ValueType& item)
{
	if (size_ == 0) { return false; }
	removeHead(item);
	return true;
}

template<class T, class EnqPolicy>
void ConcurrentQueue<T, EnqPolicy>::removeHead(ValueType& item)
{
	SequenceType holder;
	// Move the item out of elements_ in a manner that will not throw.
	holder.splice(holder.begin(), elements_, elements_.begin());
	// Record the change in the length of elements_.
	--size_;
	queueNotFull_.notify_one();
	assignItem(item, holder.front());
	used_ -= detail::memoryUsage(item);
}

template<class T, class EnqPolicy>
void ConcurrentQueue<T, EnqPolicy>::assignItem(T& item, const T& element)
{
	item = element;
}

template<class T, class EnqPolicy>
void ConcurrentQueue<T, EnqPolicy>::assignItem(std::pair<T, size_t>& item, const T& element)
{
	item.first = element;
	item.second = elementsDropped_;
	elementsDropped_ = 0;
}

template<class T, class EnqPolicy>
bool ConcurrentQueue<T, EnqPolicy>::isFull() const
{
	if (size_ >= capacity_ || used_ >= memory_) { return true; }
	return false;
}
}  // namespace artdaq

#endif /* artdaq_core_Core_ConcurrentQueue_hh */
