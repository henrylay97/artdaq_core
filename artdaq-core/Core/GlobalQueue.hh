#ifndef artdaq_core_Core_GlobalQueue_hh
#define artdaq_core_Core_GlobalQueue_hh

#include <memory>
#include "artdaq-core/Core/ConcurrentQueue.hh"
#include "artdaq-core/Data/RawEvent.hh"

namespace artdaq {
/**
   * \brief A smart pointer to a RawEvent object
   */
typedef std::shared_ptr<RawEvent> RawEvent_ptr;
/**
   * \brief A ConcurrentQueue of RawEvent objects
   */
typedef artdaq::ConcurrentQueue<RawEvent_ptr> RawEventQueue;

/**
   * \brief The first thread to call getGlobalQueue() causes the creation of
   * the queue. The queue will be destroyed at static destruction time.
   * \param maxSize Maximum number of elements in the queue
   * \return Reference to the global RawEventQueue
   */
RawEventQueue& getGlobalQueue(RawEventQueue::SizeType maxSize = std::numeric_limits<RawEventQueue::SizeType>::max());
}  // namespace artdaq

#endif /* artdaq_core_Core_GlobalQueue_hh */
