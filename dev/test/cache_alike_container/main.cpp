/*
	Shrimp

	Unit test for cache_alike_container.
*/

#include <catch/catch.hpp>

#include <shrimp/cache_alike_container.hpp>
#include <shrimp/key_multivalue_queue.hpp>

#include <string>

using namespace std::string_literals;

TEST_CASE( "[single-value] simple insert" )
{
	using namespace shrimp;

	using cache_t = cache_alike_container_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );

	cache.insert( "first", "First" );
	REQUIRE( !cache.empty() );

	{
		auto l = cache.lookup( "first" );
		REQUIRE( l );
		REQUIRE( l->key() == "first" );
		REQUIRE( l->value() == "First" );
	}

	{
		auto l = cache.lookup( "second" );
		REQUIRE( !l );
	}

	cache.insert( "second", "Second" );
	{
		auto l = cache.lookup( "second" );
		REQUIRE( l );
		REQUIRE( l->key() == "second" );
		REQUIRE( l->value() == "Second" );
	}
}

TEST_CASE( "[single-value] insert-erase-insert" )
{
	using namespace shrimp;

	using cache_t = cache_alike_container_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );

	cache.insert( "first", "First" );
	REQUIRE( !cache.empty() );

	{
		auto l = cache.lookup( "first" );
		REQUIRE( l );
		REQUIRE( l->value() == "First" );

		cache.erase( l.value() );
		REQUIRE( cache.empty() );

		REQUIRE( !cache.lookup( "first" ) );
	}

	cache.insert( "first", "First-2" );
	REQUIRE( !cache.empty() );

	{
		auto l = cache.lookup( "first" );
		REQUIRE( l );
		REQUIRE( l->value() == "First-2" );
	}
}

TEST_CASE( "[single-value] simple oldest" )
{
	using namespace shrimp;

	using cache_t = cache_alike_container_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );
	REQUIRE( !cache.oldest() );

	cache.insert( "first", "First" );
	cache.insert( "second", "Second" );

	auto l = cache.oldest();
	REQUIRE( l );
	REQUIRE( l->key() == "first" );
}

TEST_CASE( "[single-value] oldest with update_access_time" )
{
	using namespace shrimp;

	using cache_t = cache_alike_container_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );
	REQUIRE( !cache.oldest() );

	cache.insert( "first", "First" );
	cache.insert( "second", "Second" );

	auto l = cache.oldest();
	REQUIRE( l );
	REQUIRE( l->key() == "first" );

	cache.update_access_time( *l );

	auto l2 = cache.oldest();
	REQUIRE( l2 );
	REQUIRE( l2->key() == "second" );
}

TEST_CASE( "[single-value] several update_access_time with one item only" )
{
	using namespace shrimp;

	using cache_t = cache_alike_container_t<std::string, std::string>;

	cache_t cache;

	cache.insert( "first", "First" );

	for( int i = 0; i != 1000; ++i )
	{
		auto l = cache.oldest();
		REQUIRE( l );
		REQUIRE( l->key() == "first" );

		cache.update_access_time( *l );

		auto l2 = cache.oldest();
		REQUIRE( l2 );
		REQUIRE( l2->key() == "first" );

		// Just to use some memory.
		cache.insert( "two", "Two" );
		cache.insert( "three", "Three" );
		cache.erase( cache.lookup( "two" ).value() );
		cache.erase( cache.lookup( "three" ).value() );
	}
}

TEST_CASE( "[multi-value] simple insert" )
{
	using namespace shrimp;

	using cache_t = key_multivalue_queue_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );

	cache.insert( "first", "First" );
	REQUIRE( !cache.empty() );

	REQUIRE( cache.has_key( "first" ) );
	REQUIRE( 1u == cache.unique_keys() );

	REQUIRE( !cache.has_key( "second" ) );
	
	cache.insert( "second", "Second" );
	REQUIRE( cache.has_key( "second" ) );
	REQUIRE( 2u == cache.unique_keys() );
}

TEST_CASE( "[multi-value] simple oldest" )
{
	using namespace shrimp;

	using cache_t = key_multivalue_queue_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );
	REQUIRE( !cache.oldest() );

	cache.insert( "first", "First" );
	cache.insert( "second", "Second" );

	auto l = cache.oldest();
	REQUIRE( l );
	REQUIRE( l->key() == "first" );
}

TEST_CASE( "[multi-value] oldest with erase" )
{
	using namespace shrimp;

	using cache_t = key_multivalue_queue_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );
	REQUIRE( !cache.oldest() );

	cache.insert( "first", "F1" );
	cache.insert( "second", "S1" );
	cache.insert( "first", "F2" );
	cache.insert( "third", "T1" );
	cache.insert( "second", "S2" );
	cache.insert( "first", "F3" );

	REQUIRE( 3u == cache.unique_keys() );

	const auto check = [&cache](auto key, auto value) {
		auto l = cache.oldest();
		REQUIRE( l );
		REQUIRE( l->key() == key );
		REQUIRE( l->value() == value );
		cache.erase( std::move(*l) );
	};

	check( "first"s, "F1"s );
	REQUIRE( 3u == cache.unique_keys() );
	check( "second"s, "S1"s );
	REQUIRE( 3u == cache.unique_keys() );
	check( "first"s, "F2"s );
	REQUIRE( 3u == cache.unique_keys() );
	check( "third"s, "T1"s );
	REQUIRE( 2u == cache.unique_keys() );
	check( "second"s, "S2"s );
	REQUIRE( 1u == cache.unique_keys() );
	check( "first"s, "F3"s );
	REQUIRE( 0u == cache.unique_keys() );
}

TEST_CASE( "[multi-value] extract oldest" )
{
	using namespace shrimp;

	using cache_t = key_multivalue_queue_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );
	REQUIRE( !cache.oldest() );

	cache.insert( "first", "F1" );
	cache.insert( "second", "S1" );
	cache.insert( "first", "F2" );
	cache.insert( "third", "T1" );
	cache.insert( "second", "S2" );
	cache.insert( "first", "F3" );

	REQUIRE( 3u == cache.unique_keys() );

	{
		auto l = cache.oldest();
		std::vector<std::string> items;
		cache.extract_values_for_key_of( std::move(l.value()),
				[&items](auto && v) { items.push_back(std::move(v)); } );
		REQUIRE( items == std::vector<std::string>{ { "F1"s, "F2"s, "F3"s } } );
	}

	REQUIRE( 2u == cache.unique_keys() );

	const auto check = [&cache](auto key, auto value) {
		auto l = cache.oldest();
		REQUIRE( l );
		REQUIRE( l->key() == key );
		REQUIRE( l->value() == value );
		cache.erase( std::move(*l) );
	};

	check( "second"s, "S1"s );
	REQUIRE( 2u == cache.unique_keys() );
	check( "third"s, "T1"s );
	REQUIRE( 1u == cache.unique_keys() );
	check( "second"s, "S2"s );
	REQUIRE( 0u == cache.unique_keys() );
}

TEST_CASE( "[multi-value] extract oldest-2" )
{
	using namespace shrimp;

	using cache_t = key_multivalue_queue_t<std::string, std::string>;

	cache_t cache;

	REQUIRE( cache.empty() );
	REQUIRE( !cache.oldest() );

	cache.insert( "first", "F1" );
	cache.insert( "second", "S1" );
	cache.insert( "first", "F2" );
	cache.insert( "third", "T1" );
	cache.insert( "second", "S2" );
	cache.insert( "first", "F3" );

	REQUIRE( 3u == cache.unique_keys() );

	const auto check = [&cache](std::vector<std::string> expected) {
		auto l = cache.oldest();
		std::vector<std::string> items;
		cache.extract_values_for_key_of( std::move(l.value()),
				[&items](auto && v) { items.push_back(std::move(v)); } );
		REQUIRE( items == expected );
	};

	check( { "F1"s, "F2"s, "F3"s } );
	REQUIRE( 2u == cache.unique_keys() );
	check( { "S1"s, "S2"s } );
	REQUIRE( 1u == cache.unique_keys() );
	check( { "T1"s } );
	REQUIRE( 0u == cache.unique_keys() );

	REQUIRE( cache.empty() );
}

