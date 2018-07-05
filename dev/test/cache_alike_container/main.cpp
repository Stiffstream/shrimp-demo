/*
	Shrimp

	Unit test for cache_alike_container.
*/

#include <catch/catch.hpp>

#include <shrimp/cache_alike_container.hpp>

#include <string>

TEST_CASE( "simple insert" )
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

TEST_CASE( "insert-erase-insert" )
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

TEST_CASE( "simple oldest" )
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

TEST_CASE( "oldest with update_access_time" )
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

TEST_CASE( "several update_access_time with one item only" )
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

