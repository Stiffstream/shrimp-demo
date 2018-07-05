/*
	Shrimp
*/

#include <shrimp/http_server.hpp>
#include <shrimp/response_common.hpp>
#include <shrimp/utils.hpp>
#include <shrimp/a_transform_manager.hpp>

#include <algorithm>
#include <optional>
#include <cctype>

using namespace std::literals;

namespace shrimp
{

namespace /* anonymous */
{

//! Get image_format_t from file extension.
/*!
 * \return empty value if format can't be detected.
 */
[[nodiscard]] std::optional< image_format_t >
image_format_from_extension( std::string_view ext ) noexcept
{
	const auto compare_with = [&](std::string_view what) {
		using namespace std;
		const auto pred = [](auto ch1, auto ch2) {
			return tolower(ch1) == tolower(ch2);
		};
		return equal( begin(ext), end(ext), begin(what), end(what), pred );
	};

	if( compare_with("jpg") || compare_with("jpeg") )
		return image_format_t::jpeg;
	else if( compare_with("png") )
		return image_format_t::png;
	else if( compare_with("gif") )
		return image_format_t::gif;
	else
		return {};
}

template < typename Handler >
void
try_to_handle_request(
	Handler handler,
	restinio::request_handle_t & req )
{
	try
	{
		handler();
	}
	catch( const std::exception & /*ex*/ )
	{
		do_400_response( std::move( req ) );
	}
}

//
// handle_resize_op_request()
//

void
handle_resize_op_request(
	const so_5::mbox_t & req_handler_mbox,
	image_format_t image_format,
	const restinio::query_string_params_t & qp,
	restinio::request_handle_t req )
{
	try_to_handle_request(
		[&]{
			auto op_params = transform::resize_params_t::make(
				restinio::opt_value< std::uint32_t >( qp, "width" ),
				restinio::opt_value< std::uint32_t >( qp, "height" ),
				restinio::opt_value< std::uint32_t >( qp, "max" ) );

			transform::resize_params_constraints_t{}.check( op_params );

			std::string image_path{ req->header().path() };
			so_5::send<
						so_5::mutable_msg<a_transform_manager_t::resize_request_t>>(
					req_handler_mbox,
					std::move(req),
					std::move(image_path),
					image_format,
					op_params );
		},
		req );
}

namespace {

[[nodiscard]] bool
has_illegal_path_components( restinio::string_view_t path ) noexcept
{
	static constexpr auto npos = restinio::string_view_t::npos;
	return npos != path.find( ".." ) || npos != path.find( "//" );
}

} /* namespace anonymous */

void
add_transform_op_handler(
	const app_params_t & app_params,
	http_req_router_t & router,
	so_5::mbox_t req_handler_mbox )
{
	router.http_get(
		R"(/:path(.*)\.:ext(.{3,4}))",
			restinio::path2regex::options_t{}.strict( true ),
			[req_handler_mbox, &app_params]( auto req, auto params )
			{
				if( has_illegal_path_components( req->header().path() ) )
				{
					// Invalid path.
					return do_400_response( std::move( req ) );
				}

				const auto opt_image_format = image_format_from_extension(
						params[ "ext" ] );

				if( !opt_image_format )
				{
					// We do not handle files without extension or with
					// unknown extension.
					return do_400_response( std::move( req ) );
				}

				if( req->header().query().empty() )
				{
					// No query string => serve original file.
					return serve_as_regular_file(
							app_params.m_storage.m_root_dir,
							std::move( req ),
							*opt_image_format );
				}

				// Query params.
				const auto qp = restinio::parse_query( req->header().query() );

				if( "resize" != restinio::value_or( qp, "op"sv, ""sv ) )
				{
					// Only resize operation is supported.
					return do_400_response( std::move( req ) );
				}

				handle_resize_op_request(
						req_handler_mbox,
						*opt_image_format,
						qp,
						std::move( req ) );

				return restinio::request_accepted();
	} );
}

} /* namespace anonymous */

std::unique_ptr< http_req_router_t >
make_router(
	const app_params_t & params,
	so_5::mbox_t req_handler_mbox )
{
	auto router = std::make_unique< http_req_router_t >();

	add_transform_op_handler( params, *router, req_handler_mbox );

	return router;
}

} /* namespace shrimp */

