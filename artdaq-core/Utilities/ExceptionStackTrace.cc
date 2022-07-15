#include "TRACE/trace.h"
#define TRACE_NAME "ExceptionStackTrace"

#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <libgen.h>
#include <iostream>
#include <regex>
#include <sstream>

#include "ExceptionStackTrace.hh"

namespace artdaq::debug {
#define SKIP_HEAD 2
#define SKIP_TAIL 3

StackTraceCollector& getStackTraceCollector()
{
	static StackTraceCollector collector = StackTraceCollector();
	return collector;
}

std::string StackTrace::demangle(std::string const& symbol)
{
	int status;
	std::unique_ptr<char, void (*)(void*)> demangled_symbol(abi::__cxa_demangle(symbol.c_str(), nullptr, nullptr, &status), &std::free);
	return status != 0 ? symbol : &*demangled_symbol;
}

std::string Trace::print() const
{
	std::ostringstream os;
	os << '#' << index_ << " "
	   //   << ' ' << std::setw(16) << address_ << " in "
	   << filename_ << " : " << function_ << " + "
	   << "0x" << std::hex << offset_;
	return os.str();
}

void Trace::resolve()
{
	auto m = std::smatch();

	if (std::regex_search(symbol_, m, std::regex{"(\\S*)\\((|\\S+)\\)\\s\\[(0x\\S+)\\]"}) && m.size() == 4)
	{
		filename_ = m[1];
		function_ = m[2];
		address_ = static_cast<uintptr_t>(stoull(m[3], 0, 16));
		if (std::regex_search(function_, m, std::regex{"(\\S+)\\+(\\S+)"}) && m.size() == 3)
		{
			std::string offstr = m[2];
			function_ = StackTrace::demangle(m[1]);
			offset_ = static_cast<uintptr_t>(stoull(offstr, 0, 16));
		}
	}
	else  // slow parse
	{
		if (std::regex_search(symbol_, m, std::regex{"(.*)\\("}) && m.size() == 2)
			filename_ = m[1];

		if (std::regex_search(symbol_, m, std::regex{"[(](.*)[+]"}) && m.size() == 2)
			function_ = StackTrace::demangle(m[1]);

		if (std::regex_search(symbol_, m, std::regex{"[+](0x\\S+)\\)"}) && m.size() == 2)
			offset_ = static_cast<uintptr_t>(stoull(m[1], 0, 16));

		if (std::regex_search(symbol_, m, std::regex{"\\[(0x\\S+)\\]"}) && m.size() == 2)
			address_ = static_cast<uintptr_t>(stoull(m[1], 0, 16));
	}
}

StackTrace::StackTrace(std::string type_name)
    : type_name_{std::move(type_name)}, traces_uptr_{nullptr}, size_{::backtrace(frames_, sizeof frames_ / sizeof(void*))}
{
}

void StackTrace::resolve()
{
	traces_uptr_ = std::make_unique<traces_t>();

	if (0 == size_)
		return;

	traces_uptr_->reserve(size_);

	char** symbols = ::backtrace_symbols(frames_, size_);

	for (auto i = SKIP_HEAD; i < size_ - SKIP_TAIL; i++)
		traces_uptr_->emplace_back(size_ - SKIP_TAIL - i, symbols[size_ - i - 1]);

	free(symbols);

	for (auto& trace : *traces_uptr_)
		trace.resolve();
}

std::string StackTrace::print() const
{
	if (!traces_uptr_)
		return "Error: Unresolved StackTrace, call resolve() first.";

	if (0 == size_)
	{
		std::cout << "Error: possibly corrupt stack.";
	}
	std::ostringstream os;
	os << "Caught a \"" << StackTrace::demangle(type_name_) << "\" exception.\n";

	os << "Stack Trace: \n";

	for (auto const& trace : *traces_uptr_)
		os << trace << "\n";

	return os.str();
}

}  // namespace artdaq::debug

extern "C" {
#ifndef __clang__
void __cxa_throw(void* ex, void* info, void (*dest)(void*))
{
	artdaq::debug::getStackTraceCollector().collect_stacktrace(static_cast<std::type_info*>(info)->name());

	__cxa_throw_t* rethrow __attribute__((noreturn)) = (__cxa_throw_t*)dlsym(RTLD_NEXT, "__cxa_throw");

	rethrow(ex, info, dest);
}
#else
__attribute__((noreturn)) void __cxa_throw(void* ex, std::type_info* info, void (*dest)(void*))
{
	artdaq::debug::getStackTraceCollector().collect_stacktrace(info->name());

	auto* rethrow = (__cxa_throw_t*)dlsym(RTLD_NEXT, "__cxa_throw");

	rethrow(ex, info, dest);
}
#endif
}
