#ifndef EXCEPTIONSTACKTRACE_H
#define EXCEPTIONSTACKTRACE_H

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

extern "C" {

/**
 * \brief Overloads _cxa_throw and captures stack frames within the context of the thrown exception
 */
#ifndef __clang__
typedef void(__cxa_throw_t)(void*, void*, void (*)(void*));
void __cxa_throw(void*, void*, void (*)(void*));
#else   //__clang__
typedef __attribute__((noreturn)) void(__cxa_throw_t)(void*, std::type_info*, void (*)(void*));
__attribute__((noreturn)) void __cxa_throw(void*, std::type_info*, void (*)(void*));
#endif  //__clang__
}

namespace artdaq::debug {

/**
 * \brief Represents one line of the stack trace message
 */
class Trace
{
public:
	/**
	 * \brief Constructor
	 * \param index   The position in a backtrace list
	 * \param symbol  A backtrace symbol
	 */
	explicit Trace(size_t index, std::string symbol)
	    : index_{index}, symbol_{std::move(symbol)}, address_{0}, filename_{"unresolved"}, function_{"unresolved"}, offset_{0}
	{
	}

	//default
	Trace(Trace&&) = default;  ///< Default Move Constructor

	//deleted
	Trace(const Trace&) = delete;             ///< Copy Constructor is deleted
	Trace& operator=(const Trace&) = delete;  ///< Copy Assignment operator is deleted
	Trace& operator=(Trace&&) = delete;       ///< Move Assignment operator is deleted

	/**
	 * \brief Produces a one-line summary
	 */
	std::string print() const;

	/**
	 * \brief Reads and demangles backtrace symbols
	 */
	void resolve();

private:
	/**
	 * \brief The position in the backtrace list
	 */
	size_t index_;
	/**
	 * \brief The backtrace symbol as given by ::backtrace_symbols
	 */
	std::string symbol_;
	/**
	 * \brief The address of a backtrace symbol
	 */
	uintptr_t address_;
	/**
	 * \brief The name of a file or a library the backtrace symbol is originating from
	 */
	std::string filename_;
	/**
	 * \brief The demangled function name
	 */
	std::string function_;
	/**
	 * \brief The demangled  offset within a function
	 */
	uintptr_t offset_;
};

inline std::ostream& operator<<(std::ostream& os, Trace const& trace)
{
	os << trace.print();
	return os;
}

/**
 * \brief Represents the entire stack trace message
 */
class StackTrace
{
public:
	using traces_t = std::vector<Trace>;  ///< Trace collection type

	/**
	 * \brief Constructor
	 * \param type_name  The mangled type of the thrown exception
	 */
	explicit StackTrace(std::string type_name);

	/**
	 * \brief Produces a stack trace summary
	 */
	std::string print() const;
	/**
	 * \brief Reads and demangles backtrace symbols
	 */
	void resolve();

	//default
	StackTrace(StackTrace&&) = default;             ///< Default Move Constructor
	StackTrace& operator=(StackTrace&&) = default;  ///< Default move assignment operator

	//deleted
	StackTrace(const StackTrace&) = delete;             ///< Copy Constructor is deleted
	StackTrace& operator=(const StackTrace&) = delete;  ///< Copy assignment operator is deleted

	/**
	 * \brief Demangles backtrace symbols
	 */
	static std::string demangle(std::string const& symbol);

private:
	/**
	 * \brief The mangled type of the thrown exception
	 */
	std::string type_name_;
	/**
	 * \brief The collection of Traces
	 */
	std::unique_ptr<traces_t> traces_uptr_;
	/**
	 * \brief The number of frames stored in the frames_
	 */
	int size_;
	/**
	 * \brief Actual stack frames captured by the oveloaded "__cxa_throw" function
	 */
	void* frames_[1024];
};

inline std::ostream& operator<<(std::ostream& os, StackTrace const& stack_trace)
{
	os << stack_trace.print();
	return os;
}

/**
 * \brief Collects stack traces from different threads
 */
class StackTraceCollector
{
public:
	using stacktrace_map_t = std::unordered_map<std::thread::id, StackTrace>;  ///< Map relating Thread IDs to their StackTraces

	/**
	 * \brief Constructor
	 */
	StackTraceCollector()
	    : stack_traces_{}, stack_traces_mutex_{} {}

	//deleted
	StackTraceCollector(const StackTraceCollector&) = delete;             ///< Copy Constructor is deleted
	StackTraceCollector& operator=(const StackTraceCollector&) = delete;  ///< Copy Assignment is deleted
	StackTraceCollector(StackTraceCollector&&) = delete;                  ///< Move Constructor is deleted
	StackTraceCollector& operator=(StackTraceCollector&&) = delete;       ///< Move Assignment is deleted

	/**
	 * \brief Adds a stacktrace to the stack_traces_ map
	 */
	template<typename... Args>
	void collect_stacktrace(Args&&... args)
	{
		std::lock_guard<std::mutex> lg(stack_traces_mutex_);
		stack_traces_.insert_or_assign(std::this_thread::get_id(), StackTrace(std::forward<Args>(args)...));
	}

	/**
	 * \brief Produces a stack trace summary
	 */
	std::string print_stacktrace()
	{
		try
		{
			std::lock_guard<std::mutex> lg(stack_traces_mutex_);
			auto& stack_trace = stack_traces_.at(std::this_thread::get_id());
			stack_trace.resolve();
			return stack_trace.print();
		}
		catch (...)
		{
			return "Error: possibly corrupt stack.";
		}
	}

private:
	/**
	 * \brief A map of stack traces keyed by the thread id
	 */
	stacktrace_map_t stack_traces_;

	/**
	 * \brief A mutes to guard the stack_traces_ variable
	 */
	mutable std::mutex stack_traces_mutex_;
};

StackTraceCollector& getStackTraceCollector();
}  // namespace artdaq::debug
#endif /* #ifndef  EXCEPTIONSTACKTRACE_H*/
