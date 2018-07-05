/*
	Shrimp

	Unit test for transform/utils.
*/

#include <catch/catch.hpp>

#include <shrimp/utils.hpp>

TEST_CASE( "make_full_path" , "[make_full_path]" )
{
	using namespace shrimp;

	REQUIRE( make_full_path(".", "/123.jpeg" ) == "./123.jpeg");
	REQUIRE( make_full_path("~/media/pics", "/summer2018/logo.jpeg" ) ==
							"~/media/pics/summer2018/logo.jpeg");
}

