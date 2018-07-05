/*
	Shrimp

	Unit test for transform/utils.
*/

// Fix for debug build:  ‘__assert_fail’ was not declared in this scope
// somewhere in fmt.
#include <cassert>

#include <catch/catch.hpp>

#include <shrimp/transforms.hpp>

TEST_CASE( "scale_second_component" , "[scale_second_component]" )
{
	using namespace shrimp::transform;

	REQUIRE( 1 == scale_second_component(1, 1, 1 ) );
	REQUIRE( 2 == scale_second_component(1, 1, 2 ) );
	REQUIRE( 42 == scale_second_component(1, 1, 42 ) );
	REQUIRE( 13 == scale_second_component(1, 1, 13 ) );

	// (100, 150) => (50, 75 )
	REQUIRE( 50 == scale_second_component(150, 75, 100 ) );
	REQUIRE( 75 == scale_second_component(100, 50, 150 ) );

	// (100, 150) => (300, 450 )
	REQUIRE( 300 == scale_second_component(150, 450, 100 ) );
	REQUIRE( 450 == scale_second_component(100, 300, 150 ) );

	// (101, 173) => (263, 450* )
	REQUIRE( 263 == scale_second_component(173, 450, 101 ) );

	// (101, 173) => (300*, 514 )
	REQUIRE( 514 == scale_second_component(101, 300, 173 ) );

	// (1000, 10) => (10, 1 )
	REQUIRE( 1 == scale_second_component(1000, 10, 10 ) );

	REQUIRE_THROWS( scale_second_component(100, 0, 10 ) );
}

TEST_CASE( "calculate_result_size resize" , "[calculate_result_size][resize]" )
{
	using namespace shrimp::transform;

	{
		Magick::Geometry original_size{ 1, 1 };
		Magick::Geometry result_size;

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( 1, std::nullopt, std::nullopt ) ) );

		REQUIRE( 1 == result_size.width() );
		REQUIRE( 1 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, 1, std::nullopt ) ) );

		REQUIRE( 1 == result_size.width() );
		REQUIRE( 1 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, std::nullopt, 1 ) ) );

		REQUIRE( 1 == result_size.width() );
		REQUIRE( 1 == result_size.height() );
	}

	{
		Magick::Geometry original_size{ 100, 100 };
		Magick::Geometry result_size;

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( 125, std::nullopt, std::nullopt ) ) );

		REQUIRE( 125 == result_size.width() );
		REQUIRE( 125 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, 220, std::nullopt ) ) );

		REQUIRE( 220 == result_size.width() );
		REQUIRE( 220 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, std::nullopt, 1000 ) ) );

		REQUIRE( 1000 == result_size.width() );
		REQUIRE( 1000 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( 111, std::nullopt, std::nullopt ) ) );

		REQUIRE( 111 == result_size.width() );
		REQUIRE( 111 == result_size.height() );
	}

	{
		Magick::Geometry original_size{ 600, 400 };
		Magick::Geometry result_size;

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( 60, std::nullopt, std::nullopt ) ) );

		REQUIRE( 60 == result_size.width() );
		REQUIRE( 40 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, std::nullopt, 600 ) ) );

		REQUIRE( 600 == result_size.width() );
		REQUIRE( 400 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, std::nullopt, 610 ) ) );

		REQUIRE( 610 == result_size.width() );
		REQUIRE( 407 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, 40, std::nullopt ) ) );

		REQUIRE( 60 == result_size.width() );
		REQUIRE( 40 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( 1200, std::nullopt, std::nullopt ) ) );

		REQUIRE( 1200 == result_size.width() );
		REQUIRE( 800 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, std::nullopt, 900 ) ) );

		REQUIRE( 900 == result_size.width() );
		REQUIRE( 600 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, 200, std::nullopt ) ) );

		REQUIRE( 300 == result_size.width() );
		REQUIRE( 200 == result_size.height() );

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( 400, std::nullopt, std::nullopt ) ) );

		REQUIRE( 400 == result_size.width() );
		REQUIRE( 267 == result_size.height() );
	}

	{
		Magick::Geometry original_size{ 2400, 400 };
		Magick::Geometry result_size;

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( 3, std::nullopt, std::nullopt ) ) );

		REQUIRE( 3 == result_size.width() );
		REQUIRE( 1 == result_size.height() ); // At least 1.

		REQUIRE_NOTHROW(
			result_size =
				calculate_result_size(
					original_size,
					resize_params_t::make( std::nullopt, std::nullopt, 3 ) ) );

		REQUIRE( 3 == result_size.width() );
		REQUIRE( 1 == result_size.height() ); // At least 1.
	}
}
