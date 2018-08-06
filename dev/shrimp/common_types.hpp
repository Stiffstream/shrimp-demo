/*
	Shrimp
*/

/*!
	Some common types used in Shrimp.
*/

#pragma once

#include <chrono>
#include <ctime>
#include <iosfwd>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <so_5/all.hpp>

#include <fmt/format.h>

#include <Magick++.h>

namespace shrimp
{

//
// image_format_t
//

//! Image formats supported by shrimp.
enum class image_format_t
{
	gif,
	jpeg,
	png,
};

//
// exception_t
//

//! Exception class for all exceptions thrown by Shrimp.
class exception_t
	:	public std::runtime_error
{
	using bast_type_t = std::runtime_error;
	public:
		exception_t( const char * err )
			:	bast_type_t{ err }
		{}

		exception_t( const std::string & err )
			:	bast_type_t{ err }
		{}

		exception_t( std::string_view err )
			:	bast_type_t{ std::string{ err.data(), err.size() } }
		{}

		template < typename... Args >
		exception_t( std::string_view format_str, Args &&... args )
			:	bast_type_t{ fmt::format( format_str, std::forward< Args >( args )... ) }
		{}
};

//
// datasizable_blob_t
//

//! Blob for transformed image.
struct datasizable_blob_t : public std::enable_shared_from_this< datasizable_blob_t >
{
	const void *
	data() const noexcept
	{
		return m_blob.data();
	}

	std::size_t
	size() const noexcept
	{
		return m_blob.length();
	}

	Magick::Blob m_blob;

	//! Value for `Last-Modified` http header field.
	const std::chrono::system_clock::time_point m_last_modified_at{
			std::chrono::system_clock::now() };
};

using datasizable_blob_shared_ptr_t = std::shared_ptr< datasizable_blob_t >;

inline datasizable_blob_shared_ptr_t
make_blob( Magick::Image & image )
{
	auto blob = std::make_shared< datasizable_blob_t >();
	image.write( &blob->m_blob );

	return blob;
}

//
// sobj_shptr_t
//
//! A special useful name for SObjectizer's smart pointers.
template<typename T>
using sobj_shptr_t = so_5::intrusive_ptr_t<T>;

} /* namespace shrimp */

