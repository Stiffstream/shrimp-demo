/*
	Shrimp
*/

#include <shrimp/response_common.hpp>
#include <shrimp/utils.hpp>

namespace shrimp
{

namespace /* anonymous */
{

[[nodiscard]] const char *
image_src_to_str( http_header::image_src_t image_src )
{
	using http_header::image_src_t;
	switch( image_src )
	{
		case image_src_t::cache: return "cache";
		case image_src_t::transform: return "transform";
		case image_src_t::sendfile: return "sendfile";
	}

	throw exception_t{ "unknown value for image_src: {}",
			static_cast<unsigned int>(image_src) };
}

//
// image_content_type_by_img_format
//

//! Get value for http header field `Content-Type`.
[[nodiscard]] const char *
image_content_type_from_img_format( image_format_t img_format )
{
	switch( img_format )
	{
		case image_format_t::gif: return "image/gif";
		case image_format_t::jpeg: return "image/jpeg";
		case image_format_t::png: return "image/png";
	}

	// Shrimp must not execute to this point. Possible only if new image
	// type is added and it is not supported by this function.
	throw exception_t{ "undefined image type" };
}

} /* anonymous namespace */

void
serve_transformed_image(
	restinio::request_handle_t req,
	datasizable_blob_shared_ptr_t blob,
	image_format_t img_format,
	http_header::image_src_t image_src,
	header_fields_list_t header_fields )
{
	auto resp = req->create_response();

	set_common_header_fields_for_image_resp(
			blob->m_last_modified_at,
			resp )
		.append_header(
			restinio::http_field::content_type,
			image_content_type_from_img_format( img_format )  )
		.append_header(
			http_header::shrimp_image_src,
			image_src_to_str( image_src ) )
		.set_body( std::move( blob ) );

	for( auto & hf : header_fields )
	{
		resp.append_header( std::move( hf.m_name ), std::move( hf.m_value ) );
	}

	resp.done();
}

//
// serve_as_regular_file()
//

[[nodiscard]] restinio::request_handling_status_t
serve_as_regular_file(
	const std::string & root_dir,
	restinio::request_handle_t req,
	image_format_t image_format )
{
	const auto full_path =
		make_full_path( root_dir, req->header().path() );

	try
	{
		auto sf = restinio::sendfile( full_path );

		auto resp = req->create_response();

		struct stat64 file_stat;
		const auto fstat_rc = fstat64( sf.file_descriptor(), &file_stat );

		if( 0 != fstat_rc )
		{
			throw exception_t{
				fmt::format( "unable to get file stats: {}", strerror( errno ) ) };
		}

		return set_common_header_fields_for_image_resp(
					file_stat.st_mtim.tv_sec,
					resp )
				.append_header(
					restinio::http_field::content_type,
					image_content_type_from_img_format( image_format ) )
				.append_header(
					http_header::shrimp_image_src,
					image_src_to_str( http_header::image_src_t::sendfile ) )
				.set_body( std::move( sf ) )
				.done();
	}
	catch(...)
	{
	}

	return do_404_response( std::move( req ) );
}

} /* namespace shrimp */
