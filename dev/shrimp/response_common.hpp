/*
	Shrimp
*/

/*!
	Common functions for making responses.
*/

#pragma once

#include <restinio/all.hpp>

#include <shrimp/common_types.hpp>
#include <shrimp/utils.hpp>

namespace shrimp
{

//
// set_common_header_fields()
//

template < typename RESP >
inline RESP &
set_common_header_fields( RESP & resp )
{
	resp.append_header( restinio::http_field_t::server, "Shrimp draft server" );
	resp.append_header_date_field();

	return resp;
}

//
// set_common_header_fields_for_image_resp()
//

template < typename RESP >
inline RESP &
set_common_header_fields_for_image_resp(
	std::chrono::system_clock::time_point last_modified,
	RESP & resp  )
{
	set_common_header_fields( resp )
		.append_header(
			restinio::http_field_t::last_modified,
			restinio::make_date_field_value( last_modified ) )
		.append_header( "Access-Control-Allow-Origin", "*" )
		.append_header( "Access-Control-Expose-Headers",
				"Shrimp-Processing-Time, Shrimp-Image-Src" );

	return resp;
}

namespace response_common_details {

// Should the connection be closed after sending a response?
enum class connection_status_t { autodetect, keep, close };

//
// make_response_object
//
[[nodiscard]]
inline auto
make_response_object(
	restinio::request_handle_t req,
	restinio::http_status_line_t status_line,
	connection_status_t conn_status )
{
	auto resp = req->create_response( std::move(status_line) );
	set_common_header_fields( resp );

	switch( conn_status )
	{
		case connection_status_t::autodetect :
			if( req->header().should_keep_alive() )
				resp.connection_keep_alive();
			else
				resp.connection_close();
		break;

		case connection_status_t::keep :
			resp.connection_keep_alive();
		break;

		case connection_status_t::close :
			resp.connection_close();
		break;
	}
	
	return resp;
}

} /* namespace response_common_details */

//
// do_404_response()
//

inline auto
do_404_response( restinio::request_handle_t req )
{
	return
		response_common_details::make_response_object(
				req, restinio::status_not_found(),
				response_common_details::connection_status_t::autodetect )
		.done();
}

//
// do_400_response()
//

inline auto
do_400_response( restinio::request_handle_t req )
{
	return
		response_common_details::make_response_object(
				req, restinio::status_bad_request(),
				response_common_details::connection_status_t::autodetect )
		.done();
}

//
// do_503_response()
//

inline auto
do_503_response( restinio::request_handle_t req )
{
	return
		response_common_details::make_response_object(
				req, restinio::status_service_unavailable(),
				// Not too much sense to keep the connection.
				response_common_details::connection_status_t::close )
		.done();
}

//
// do_504_response()
//

inline auto
do_504_response( restinio::request_handle_t req )
{
	return
		response_common_details::make_response_object(
				req, restinio::status_gateway_time_out(),
				// Not too much sense to keep the connection.
				response_common_details::connection_status_t::close )
		.done();
}

using header_fields_list_t = std::vector< restinio::http_header_field_t >;

namespace header_fields_list_details {

template<typename... Tail>
void append(
	header_fields_list_t & to,
	std::string_view k,
	std::string_view v,
	Tail &&...tail )
{
	to.emplace_back( k, v );
	if constexpr( 0 != sizeof...(Tail) )
		append( to, std::forward<Tail>(tail)... );
}

} /* header_fields_list_details */

template<typename... Tail>
[[nodiscard]] header_fields_list_t
make_header_fields_list(
	std::string_view k,
	std::string_view v,
	Tail &&...tail )
{
	header_fields_list_t result;
	header_fields_list_details::append(
			result, k, v, std::forward<Tail>(tail)... );
	return result;
}

namespace http_header
{

[[nodiscard]]
inline std::string_view
shrimp_total_processing_time_hf() { return { "Shrimp-Processing-Time" };
}

[[nodiscard]]
inline std::string_view
shrimp_image_src_hf() { return "Shrimp-Image-Src"; }

//! Server image source.
enum class image_src_t
{
	cache,
	transform,
	sendfile
};

} /* namespace http_header */

//
// serve_transformed_image()
//

void
serve_transformed_image(
	restinio::request_handle_t req,
	datasizable_blob_shared_ptr_t,
	image_format_t img_format,
	http_header::image_src_t image_src,
	header_fields_list_t header_fields = {} );

//
// serve_as_regular_file()
//

[[nodiscard]]
restinio::request_handling_status_t
serve_as_regular_file(
	const std::string & root_dir,
	restinio::request_handle_t req,
	image_format_t image_format );

} /* namespace shrimp */

