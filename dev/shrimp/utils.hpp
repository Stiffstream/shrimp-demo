/*
	Shrimp
*/

/*!
	Various common helpers.
*/

#pragma once

#include <string>
#include <string_view>
#include <ctime>
#include <chrono>
#include <functional>

#include <restinio/all.hpp>

#include <shrimp/common_types.hpp>

namespace shrimp
{

//! Make a full path to file.
inline std::string
make_full_path( std::string_view root_dir, std::string_view path )
{
	std::string full_path;
	full_path.reserve( root_dir.size() + path.size() );
	full_path.append( root_dir.data(), root_dir.size() );
	full_path.append( path.data(), path.size() );

	return full_path;
}

//! Make a value for HTTP-header field that have Date type.
inline std::string
make_date_http_field_value( std::time_t t )
{
	const auto tpoint = restinio::make_gmtime( t );

	std::array< char, 64 > buf;
	strftime(
		buf.data(),
		buf.size(),
		"%a, %d %b %Y %H:%M:%S GMT",
		&tpoint );

	return std::string{ buf.data() };
}

//
// measure_duration
//

//! Measure some operation duration.
template < typename F >
auto
measure_duration( F && f )
{
	using hi_clock = std::chrono::high_resolution_clock;
	const auto started_at = hi_clock::now();
	f();
	return hi_clock::now() - started_at;
}

//
// variant_visitor
//

// Helpers for working with std::variant.
// An idea and implementation is borrowed from:
// https://en.cppreference.com/w/cpp/utility/variant/visit 
template<class... Ts>
struct variant_visitor : Ts... { using Ts::operator()...; };

template<class... Ts> variant_visitor(Ts...) -> variant_visitor<Ts...>;

} /* namespace shrimp */

