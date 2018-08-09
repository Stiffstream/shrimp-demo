/*
	Shrimp
*/

#pragma once

#include <functional>
#include <optional>
#include <cstdint>
#include <tuple>

#include <Magick++.h>

#include <shrimp/utils.hpp>

namespace shrimp
{

namespace transform
{

//
// resize_params_t
//

//! Resize operation parameters.
/*!
	Only one of parameters must be defined.

	If only width parameter is set, then the original image
	is resized to an image with specified width.
	The same logic applies if the height component is defined.

	If max_side parameter is set, then the original image is
	resized to have the longest side of m_max_side value.
*/
class resize_params_t
{
public :
	//! Variants of resize mode.
	enum class mode_t { width, height, longest, keep_original };

	//! Factory method for making resize params from optional values
	//! obtained from query string.
	/*!
	 * It is possible that all parameters are empty. It means that
	 * mode_t::keep_original should be used.
	 */
	static resize_params_t
	make(
		std::optional< std::uint32_t > width,
		std::optional< std::uint32_t > height,
		std::optional< std::uint32_t > max_side )
	{
		const auto count = (width ? 1:0) + (height ? 1:0) + (max_side ? 1:0);
		if( 0 == count )
			return { keep_original_size_t{} };

		if( 1 != count )
		{
			throw exception_t{
				"resize params error: "
				"exactly one parameter must be defined" };
		}

		if( width )
			return { mode_t::width, *width };
		else if( height )
			return { mode_t::height, *height };
		else
			return { mode_t::longest, *max_side };
	}

	[[nodiscard]] auto
	mode() const noexcept { return m_mode; }

	/*!
	 * \note Throws in keep_original mode.
	 */
	[[nodiscard]] auto
	value() const
	{
		if( mode_t::keep_original == m_mode )
			throw exception_t{ "value() is undefined in keep_original mode" };

		return m_value;
	}

	[[nodiscard]] bool
	operator<( const resize_params_t & p ) const noexcept
	{
		return std::tie(m_mode, m_value) < std::tie(p.m_mode, p.m_value);
	}

private :
	struct keep_original_size_t {};

	resize_params_t( mode_t mode, std::uint32_t value )
		:	m_mode{ mode }, m_value{ value }
	{}

	resize_params_t( keep_original_size_t )
		:	m_mode{ mode_t::keep_original }, m_value{}
	{}

	mode_t m_mode;
	/*!
	 * \note This value is undefined in keep_original mode.
	 */
	std::uint32_t m_value;
};

//! Print parameters to stream (for logging).
inline std::ostream &
operator<<( std::ostream & o, const resize_params_t & p )
{
	const auto m = p.mode();

	if( resize_params_t::mode_t::keep_original == m )
		return (o << "{keep_original}");
	else
	{
		switch( p.mode() )
		{
			case resize_params_t::mode_t::width : o << "{w "; break;
			case resize_params_t::mode_t::height : o << "{h "; break;
			case resize_params_t::mode_t::longest : o << "{m "; break;
			case resize_params_t::mode_t::keep_original : std::abort(); break;
		}
		return (o << p.value() << "}");
	}
}

//
// resize_request_key_t
//

//! A compound key for resize operation.
class resize_request_key_t
{
	std::string m_path;
	image_format_t m_format;
	resize_params_t m_params;

public:
	resize_request_key_t(
		std::string path,
		image_format_t format,
		resize_params_t params )
		:	m_path{ std::move(path) }
		,	m_format{ format }
		,	m_params{ params }
	{}

	[[nodiscard]] bool
	operator<(const resize_request_key_t & o ) const noexcept
	{
		return std::tie( m_path, m_format, m_params )
				< std::tie( o.m_path, o.m_format, o.m_params );
	}

	[[nodiscard]] const std::string &
	path() const noexcept
	{
		return m_path;
	}

	[[nodiscard]] image_format_t
	format() const noexcept
	{
		return m_format;
	}

	[[nodiscard]] resize_params_t
	params() const noexcept
	{
		return m_params;
	}
};

inline std::ostream &
operator<<( std::ostream & to, const resize_request_key_t & what )
{
	const auto format_to_str = [](auto fmt) {
		const char * r = nullptr;
		switch( fmt )
		{
			case image_format_t::jpeg: r = "jpg"; break;
			case image_format_t::gif: r = "gif"; break;
			case image_format_t::png: r = "png"; break;
			case image_format_t::webp: r = "webp"; break;
		}
		return r;
	};

	return (to << "{{path " << what.path() << "} {format: "
			<< format_to_str(what.format()) << "} {params: "
			<< what.params() << "}}");
}

//
// resize_params_constraints_t
//

struct resize_params_constraints_t
{
	static constexpr std::uint32_t default_max_side = 5*1000;

	std::uint32_t m_max_side{ default_max_side };

	//! Checks if given parameters fit the constraints.
	/*!
		Throws in case of error.
	*/
	void
	check( const resize_params_t & p ) const
	{
		const auto mode_name = [](const auto m) -> const char * {
			const char * r = nullptr;
			switch( m ) {
				case resize_params_t::mode_t::width: r = "width"; break;
				case resize_params_t::mode_t::height: r = "height"; break;
				case resize_params_t::mode_t::longest: r = "max_side"; break;
				case resize_params_t::mode_t::keep_original: r = "keep_original"; break;
			}
			return r;
		};

		if( resize_params_t::mode_t::keep_original != p.mode() )
		{
			if( 0 == p.value() )
			{
				throw exception_t{
						"resize params error: {} cannot be 0",
						mode_name( p.mode() ) };
			}

			if( p.value() > m_max_side )
			{
				throw exception_t{
						"resize params error: specified {} ({}) is too big, "
						"max possible value is {}",
						mode_name( p.mode() ),
						p.value(),
						m_max_side };
			}
		}
	}
};

//
// resize()
//

/*!
	Resize a given image using given parameters.

	Before resizing a resolution of resulting image is calculated and
	if it exceeds total_pixels_limit then an exception is thrown.
*/
void
resize(
	//! Resize params.
	const resize_params_t & params,
	//! limit on the total pixels count in a result.
	std::uint64_t total_pixels_limit,
	//! A reference to image object which would be modified.
	Magick::Image & img );

//
// Utilities
//

//
// scale_second_component()
//

//! Scales second_component_source_len with the same ratio as `source_len/dest_len`.
[[nodiscard]] std::size_t
scale_second_component(
	std::size_t source_len,
	std::size_t dest_len,
	std::size_t second_component_source_len );

//
// calculate_result_size()
//

//! Calculate the size of the resulting image after resize operation.
[[nodiscard]] Magick::Geometry
calculate_result_size(
	//! Size of original image.
	Magick::Geometry original_size,
	//! Resize parameters.
	const resize_params_t & params );

} /* namespace transform */

} /* namespace shrimp */

