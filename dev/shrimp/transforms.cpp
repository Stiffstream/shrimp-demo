#include <cassert>

#include <shrimp/transforms.hpp>

namespace shrimp
{

namespace transform
{

namespace /* anonymous */
{

void
check_image_size(
	Magick::Geometry img_size,
	std::uint64_t total_pixels_limit )
{
	const auto result_pixels_count = img_size.height() * img_size.width();

	if( result_pixels_count > total_pixels_limit )
	{
		throw exception_t{
			"exceeding total_pixels_limit: ({},{}) ~ {} pixels (limit: {})",
			img_size.height(),
			img_size.width(),
			result_pixels_count,
			total_pixels_limit };
	}
}

inline std::size_t
at_least_uno( float value )
{
	return std::max( std::size_t{1u}, static_cast< std::size_t >( value ) );
}

} /* anonymous namespace */


//
// resize()
//

void
resize(
	const resize_params_t & params,
	std::uint64_t total_pixels_limit,
	Magick::Image & img )
{
	const auto result_size = calculate_result_size( img.size(), params );
	check_image_size( result_size, total_pixels_limit );

	img.resize( result_size );
}

//
// scale_second_component()
//

[[nodiscard]] std::size_t
scale_second_component(
	std::size_t source_len,
	std::size_t dest_len,
	std::size_t second_component_source_len )
{
	if( 0 == dest_len )
	{
		throw exception_t{
			"scale_second_component error: dest len cannot be 0"  };
	}
	const float scale = static_cast< float >( dest_len ) / source_len;

	return at_least_uno( std::round( second_component_source_len * scale ) );
};

//
// calculate_result_size()
//

//! Calculate the size of the resulting image after resize operation.
[[nodiscard]] Magick::Geometry
calculate_result_size(
	Magick::Geometry original_size,
	const resize_params_t & params )
{
	Magick::Geometry sz;

	switch( params.mode() )
	{
		case resize_params_t::mode_t::width :
			sz.width( params.value() );
			sz.height(
				scale_second_component(
					original_size.width(),
					params.value(),
					original_size.height() ) );
		break;

		case resize_params_t::mode_t::height :
			sz.height( params.value() );
			sz.width(
				scale_second_component(
					original_size.height(),
					params.value(),
					original_size.width() ) );
		break;

		case resize_params_t::mode_t::longest :
			if( original_size.width() > original_size.height() )
			{
				sz.width( params.value() );
				sz.height(
					scale_second_component(
						original_size.width(),
						params.value(),
						original_size.height() ) );
			}
			else
			{
				sz.height( params.value() );
				sz.width(
					scale_second_component(
						original_size.height(),
						params.value(),
						original_size.width() ) );
			}
		break;

		default :
			// Must not fall here in real aplication.
			throw exception_t{
				"bad resize parameters: none of the parameters is defined" };
	}

	return sz;
}

} /* namespace transform */

} /* namespace shrimp */

