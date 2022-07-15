#include "artdaq-core/Utilities/ExceptionHandler.hh"

#define BOOST_TEST_MODULE ExceptionHandler_t
#include "cetlib/quiet_unit_test.hpp"

#define TRACE_NAME "ExceptionHandler_t"
#include "TRACE/tracemf.h"
#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"

#include <boost/exception/all.hpp>

BOOST_AUTO_TEST_SUITE(ExceptionHandler_test)

typedef boost::error_info<struct tag_my_info, std::string> my_info;

struct my_error : virtual boost::exception, virtual std::exception
{};

BOOST_AUTO_TEST_CASE(artException)
{
	try
	{
		throw art::Exception(art::errors::Unknown, "TestException");
	}
	catch (art::Exception& e)
	{
		artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::no, "This is a test of art::Exception");
		BOOST_REQUIRE_EXCEPTION(
		    artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::yes, "This is another test of art::Exception"), art::Exception, [](art::Exception const& e) { return e.category() == e.codeToString(art::errors::Unknown); });
	}
}
BOOST_AUTO_TEST_CASE(cetException)
{
	try
	{
		throw cet::exception("TestException");
	}
	catch (cet::exception&)
	{
		artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::no, "This is a test of cet::exception");
		BOOST_REQUIRE_EXCEPTION(
		    artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::yes, "This is another test of cet::exception"), cet::exception, [](cet::exception const& e) { return e.category() == "TestException"; });
	}
}
BOOST_AUTO_TEST_CASE(boostException)
{
	try
	{
		throw my_error() << my_info("TestException");
	}
	catch (boost::exception&)
	{
		artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::no, "This is a test of boost::exception");
		BOOST_REQUIRE_EXCEPTION(
		    artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::yes, "This is another test of boost::exception"), boost::exception, [](boost::exception const&) { return true; });
	}
}
BOOST_AUTO_TEST_CASE(stdException)
{
	try
	{
		throw std::exception();
	}
	catch (std::exception&)
	{
		artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::no, "This is a test of std::exception");
		BOOST_REQUIRE_EXCEPTION(
		    artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::yes, "This is another test of std::exception"), std::exception, [](std::exception const&) { return true; });
	}
}
BOOST_AUTO_TEST_CASE(arbitraryThrow)
{
	try
	{
		throw int(5);
	}
	catch (...)
	{
		artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::no, "This is a test of arbitrary throw handling");
		BOOST_REQUIRE_EXCEPTION(
		    artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::yes, "This is another test of arbitrary throw handling"), int, [](int const& e) { return e == 5; });
	}
}

BOOST_AUTO_TEST_SUITE_END()
