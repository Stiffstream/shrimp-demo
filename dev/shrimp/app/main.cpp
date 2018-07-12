/*
	shrimp.app
*/

#include <shrimp/app_params.hpp>

#include <shrimp/http_server.hpp>
#include <shrimp/a_transform_manager.hpp>
#include <shrimp/a_transformer.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <clara/clara.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/ansicolor_sink.h>

#include <stdexcept>
#include <iostream>

namespace /* anonymous */
{

enum class sobj_tracing_t { off, on };

//! Application arg parser.
struct app_args_t
{
	bool m_help{ false };
	sobj_tracing_t m_sobj_tracing{ sobj_tracing_t::off };

	shrimp::app_params_t m_app_params;

	[[nodiscard]] static app_args_t
	parse( int argc, const char * argv[] )
	{
		using namespace clara;

		app_args_t result;
		std::uint16_t ip_version = static_cast<std::uint16_t>(
				result.m_app_params.m_http_server.m_ip_version);
		bool sobj_tracing = false;

		const auto make_opt = [](auto & val,
				const char * name, const char * short_name, const char * long_name,
				const char * description) {
			return Opt( val, name )[ short_name ][ long_name ]
					( fmt::format( description, val ) );
		};

		auto cli = make_opt(
					result.m_app_params.m_http_server.m_address, "address",
					"-a", "--address",
					"address to listen (default: {})" )
			| make_opt(
					result.m_app_params.m_http_server.m_port, "port",
					"-p", "--port",
					"port to listen (default: {})" )
			| make_opt(
					ip_version, "ip-version",
					"-P", "--ip-version",
					"IP version to use (4 or 6) (default: {})" )
			| make_opt(
					result.m_app_params.m_storage.m_root_dir, "images-path",
					"-i", "--images",
					"Path for searching images (default: {})" )
			| Opt( sobj_tracing )
					[ "--sobj-tracing" ]
					( "Turn SObjectizer's message delivery tracing on" )
			| Help(result.m_help);

		auto parse_result = cli.parse( Args(argc, argv) );
		if( !parse_result )
			throw shrimp::exception_t{
					"Invalid command-line arguments: {}",
					parse_result.errorMessage() };

		if( result.m_help )
			std::cout << cli << std::endl;

		if( !(4 == ip_version || 6 == ip_version) )
			throw shrimp::exception_t{
					"Invalid value for IP version: {}", ip_version };
		else
			result.m_app_params.m_http_server.m_ip_version =
					static_cast<shrimp::http_server_params_t::ip_version_t>(
							ip_version );

		if( sobj_tracing )
			result.m_sobj_tracing = sobj_tracing_t::on;

		return result;
	}
};

[[nodiscard]]
spdlog::sink_ptr
make_logger_sink()
{
	auto sink = std::make_shared< spdlog::sinks::ansicolor_stdout_sink_mt >();
	sink->set_level( spdlog::level::trace );
	return sink;
}

[[nodiscard]]
std::shared_ptr<spdlog::logger>
make_logger(
	const std::string & name,
	spdlog::sink_ptr sink,
	spdlog::level::level_enum level = spdlog::level::trace )
{
	auto logger = std::make_shared< spdlog::logger >( name, std::move(sink) );
	logger->set_level( level );
	logger->flush_on( level );
	return logger;
}

[[nodiscard]]
auto
calculate_thread_count()
{
	struct result_t {
		unsigned int m_io_threads;
		unsigned int m_worker_threads;
	};

	constexpr unsigned int max_io_threads = 2u;
	const unsigned int cores = std::thread::hardware_concurrency();
	if( cores < max_io_threads * 3u )
		return result_t{ 1u, cores };
	else
		return result_t{ max_io_threads, cores - max_io_threads };
}

//
// spdlog_sobj_tracer_t
//

// Helper class for redirecting SObjectizer message delivery tracer
// to spdlog::logger.
class spdlog_sobj_tracer_t : public so_5::msg_tracing::tracer_t
{
	std::shared_ptr<spdlog::logger> m_logger;

	public:
		spdlog_sobj_tracer_t(
			std::shared_ptr<spdlog::logger> logger )
			:	m_logger{ std::move(logger) }
		{}

		virtual void
		trace( const std::string & what ) noexcept override
		{
			m_logger->trace( what );
		}

		[[nodiscard]]
		static so_5::msg_tracing::tracer_unique_ptr_t
		make( spdlog::sink_ptr sink )
		{
			return std::make_unique<spdlog_sobj_tracer_t>(
					make_logger( "sobjectizer", std::move(sink) ) );
		}
};

[[nodiscard]]
so_5::mbox_t
create_agents(
	spdlog::sink_ptr logger_sink,
	const shrimp::app_params_t & app_params,
	so_5::environment_t & env,
	unsigned int worker_threads_count )
{
	using namespace shrimp;

	so_5::mbox_t manager_mbox;

	// Register main coop.
	env.introduce_coop([&]( so_5::coop_t & coop )
		{
			const auto create_one_thread_disp =
				[&]( const std::string & disp_name ) {
					return so_5::disp::one_thread::create_private_disp(
							env,
							disp_name );
				};

			auto manager = coop.make_agent_with_binder< a_transform_manager_t >(
					create_one_thread_disp( "manager" )->binder(),
					make_logger( "manager", logger_sink ) );
			manager_mbox = manager->so_direct_mbox();

			// Every worker will work on its own private dispatcher.
			for( decltype(worker_threads_count) worker{};
					worker < worker_threads_count;
					++worker )
			{
				const auto worker_name = fmt::format( "worker_{}", worker );
				auto transformer = coop.make_agent_with_binder< a_transformer_t >(
						create_one_thread_disp( worker_name )->binder(),
						make_logger( worker_name, logger_sink ),
						app_params.m_storage );

				manager->add_worker( transformer->so_direct_mbox() );
			}
		} );

	return manager_mbox;
}

void
run_app(
	const shrimp::app_params_t & params,
	sobj_tracing_t sobj_tracing )
{
	const auto thread_count = calculate_thread_count();
	auto logger_sink = make_logger_sink();

	// ASIO io_context must outlive sobjectizer.
	asio::io_context asio_io_ctx;

	// Launch SObjectizer and wait while balancer will be started.
	std::promise< so_5::mbox_t > manager_mbox_promise;
	so_5::wrapped_env_t sobj{
		[&]( so_5::environment_t & env ) {
			manager_mbox_promise.set_value(
					create_agents(
							logger_sink,
							params,
							env,
							thread_count.m_worker_threads ) );
		},
		[&]( so_5::environment_params_t & params ) {
			if( sobj_tracing_t::on == sobj_tracing )
				params.message_delivery_tracer(
						spdlog_sobj_tracer_t::make( logger_sink ) );
		}
	};

	// Now we can launch HTTP-server.
	// If SObjectizer is not started yet we will wait on the future::get() call.
	restinio::run(
			asio_io_ctx,
			shrimp::make_http_server_settings(
					thread_count.m_io_threads,
					params,
					manager_mbox_promise.get_future().get() ) );
}

} /* anonymous namespace */

int
main( int argc, const char * argv[] )
{
	try
	{
		const auto args = app_args_t::parse( argc, argv );

		if( !args.m_help )
		{
			run_app( args.m_app_params, args.m_sobj_tracing );
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "ERROR: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
