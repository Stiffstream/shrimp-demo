/*
	Shrimp
*/

/*!
	Http server for receiving requests.
*/

#include <so_5/all.hpp>

#include <restinio/all.hpp>
#include <restinio/router/pcre_regex_engine.hpp>

#include <shrimp/common_types.hpp>
#include <shrimp/app_params.hpp>

namespace shrimp
{

//
// http_req_router_t
//

//! Type of express router used in application.
/*!
	Use the quickest regex engine based on benchmarks: pcre.

	Max capture groups is selected to fullfill the needs of each route pattern
	used in application and be not too big. Anyway if a new route is added
	and it needs more capture groups initialization would fail.
*/
using http_req_router_t =
	restinio::router::express_router_t<
		restinio::router::pcre_regex_engine_t<
			restinio::router::pcre_traits_t<
				// Max capture groups for regex.
				5 > > >;

//
// shrimp_http_server_traits_t
//

//! Http server traits for shrim http server.
struct http_server_traits_t
	:	public restinio::default_traits_t
{
	using request_handler_t = http_req_router_t;
};

std::unique_ptr< http_req_router_t >
make_router(
	const app_params_t & params,
	so_5::mbox_t req_handler_mbox );

//
// make_http_server_settings()
//

//! Tune Shrimp HTTP-server settings to use with restinio::run.
[[nodiscard]] inline auto
make_http_server_settings(
	unsigned int thread_pool_size,
	const app_params_t & params,
	so_5::mbox_t req_handler_mbox )
{
	const auto ip_protocol = [](auto ip_ver) {
		using restinio::asio_ns::ip::tcp;
		return http_server_params_t::ip_version_t::v4 == ip_ver ?
				tcp::v4() : tcp::v6();
	};

	const auto & http_srv_params = params.m_http_server;

	return restinio::on_thread_pool< http_server_traits_t >( thread_pool_size )
			.port( http_srv_params.m_port )
			.protocol( ip_protocol(http_srv_params.m_ip_version) )
			.address( http_srv_params.m_address )
			// NOTE: it is better to configure that value.
			.handle_request_timeout( std::chrono::seconds(60) )
			.write_http_response_timelimit( std::chrono::seconds(60) )
			.request_handler( make_router( params, req_handler_mbox ) );
}

} /* namespace shrimp */

