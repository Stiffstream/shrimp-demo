/*
	Shrimp
*/

/*!
	Http server for receiving requests.
*/

#include <shrimp/common_types.hpp>
#include <shrimp/app_params.hpp>

#include <so_5/all.hpp>

#include <restinio/all.hpp>
#include <restinio/router/pcre_regex_engine.hpp>

#include <spdlog/spdlog.h>

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
// http_server_logger_t
//

//! Logger for RESTinio server.
/*
	A wrapper for spdlog logger for RESTinio server.
	For each message it check if a specified log level is enabled
	and if so the message is pushed to spdlog logger.
*/
class http_server_logger_t
{
	public:
		http_server_logger_t( std::shared_ptr<spdlog::logger> logger )
			:	m_logger{ std::move( logger ) }
		{}

		template< typename Builder >
		void
		trace( Builder && msg_builder )
		{
			log_if_enabled( spdlog::level::trace,
					std::forward<Builder>(msg_builder) );
		}

		template< typename Builder >
		void
		info( Builder && msg_builder )
		{
			log_if_enabled( spdlog::level::info,
					std::forward<Builder>(msg_builder) );
		}

		template< typename Builder >
		void
		warn( Builder && msg_builder )
		{
			log_if_enabled( spdlog::level::warn,
					std::forward<Builder>(msg_builder) );
		}

		template< typename Builder >
		void
		error( Builder && msg_builder )
		{
			log_if_enabled( spdlog::level::err,
				std::forward<Builder>(msg_builder) );
		}

	private:
		template< typename Builder >
		void
		log_if_enabled( spdlog::level::level_enum lv, Builder && msg_builder )
		{
			if( m_logger->should_log(lv) )
			{
				m_logger->log( lv, msg_builder() );
			}
		}

		//! Logger object.
		std::shared_ptr<spdlog::logger> m_logger;
};

//
// shrimp_http_server_traits_t
//

//! Http server traits for shrim http server.
struct http_server_traits_t
	:	public restinio::default_traits_t
{
	using logger_t = http_server_logger_t;
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
	std::shared_ptr<spdlog::logger> logger,
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
			.logger( std::move(logger) )
			.request_handler( make_router( params, req_handler_mbox ) );
}

} /* namespace shrimp */

