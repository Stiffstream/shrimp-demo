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
enum class restinio_tracing_t { off, on };

// Helper class to be used as strong typedef for thread count type.
// Thread count can't have zero value.
class thread_count_t final
{
public:
	using underlying_type_t = unsigned int;

private:
	static constexpr const char * const invalid_value = "Thread count can't be zero";

	struct trusty_t { underlying_type_t m_value; };

	underlying_type_t m_value;

	thread_count_t(trusty_t trusty) noexcept
		:	m_value{ trusty.m_value }
	{}

	[[nodiscard]]
	static underlying_type_t
	valid_value_or_throw( const std::variant<thread_count_t, const char *> v )
	{
		if( auto * msg = std::get_if<const char *>(&v); msg )
			throw std::runtime_error( *msg );

		return std::get<thread_count_t>(v).value();
	}

public:
	// Returns a normal object or pointer to error message in the case
	// of invalid value.
	[[nodiscard]]
	static std::variant<thread_count_t, const char *>
	try_construct( underlying_type_t value ) noexcept
	{
		if( 0u == value )
			return { invalid_value };
		else
			return { thread_count_t{ trusty_t{ value } } };
	}

	// Construct an object with checking for errors.
	// Throws an exception in case of invalid value.
	thread_count_t(underlying_type_t value)
		: m_value{ valid_value_or_throw( try_construct(value) ) }
	{}

	[[nodiscard]]
	underlying_type_t
	value() const noexcept { return m_value; }
};

//! Application arg parser.
struct app_args_t
{
	bool m_help{ false };
	sobj_tracing_t m_sobj_tracing{ sobj_tracing_t::off };
	restinio_tracing_t m_restinio_tracing{ restinio_tracing_t::off };
	spdlog::level::level_enum m_log_level{ spdlog::level::trace };

	shrimp::app_params_t m_app_params;

	std::optional<thread_count_t> m_io_threads;
	std::optional<thread_count_t> m_worker_threads;

	[[nodiscard]]
	static std::optional<thread_count_t>
	thread_count_from_env_var( const char * env_var_name )
	{
		std::optional<thread_count_t> result;

		const char * var = std::getenv( env_var_name ); 
		if( var )
		{
			try
			{
				result = restinio::cast_to<thread_count_t::underlying_type_t>(
						std::string_view{ var } );
			}
			catch( const std::exception & x )
			{
				throw shrimp::exception_t{
						"Unable to process ENV-variable {}={}: {}",
						env_var_name,
						var,
						x.what() };
			}
		}

		return result;
	}

	[[nodiscard]]
	static std::optional<spdlog::level::level_enum>
	log_level_from_str( const std::string & level_name ) noexcept
	{
		if( "off" != level_name )
		{
			const auto l = spdlog::level::from_str( level_name );
			if( spdlog::level::off != l )
				return l;
			else
				return std::nullopt;
		}
		else
			return spdlog::level::off;
	}

	[[nodiscard]]
	static auto
	make_thread_count_handler( std::optional<thread_count_t> & receiver )
	{
		return [&receiver]( thread_count_t::underlying_type_t const v ) {
			using namespace clara;
			return std::visit( shrimp::variant_visitor{
					[&receiver]( const thread_count_t & count ) {
						receiver = count;
						return ParserResult::ok( ParseResultType::Matched );
					},
					[]( const char * error_msg ) {
						return ParserResult::runtimeError( error_msg );
					} },
					thread_count_t::try_construct(v) );
		};
	}

	[[nodiscard]]
	static app_args_t
	parse( int argc, const char * argv[] )
	{
		using namespace clara;

		app_args_t result;
		std::uint16_t ip_version = static_cast<std::uint16_t>(
				result.m_app_params.m_http_server.m_ip_version);
		bool sobj_tracing = false;
		bool restinio_tracing = false;
		std::string log_level{ "trace" };

		std::optional<thread_count_t> io_threads;
		std::optional<thread_count_t> worker_threads;

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
			| Opt( restinio_tracing )
					[ "--restinio-tracing" ]
					( "Turn RESTinio tracing facility on" )
			| Opt( make_thread_count_handler(io_threads), "non-zero number" )
					[ "--io-threads" ]
					( "Count of threads for IO operations" )
			| Opt( make_thread_count_handler(worker_threads), "non-zero number" )
					[ "--worker-threads" ]
					( "Count of threads for resize operations" )
			| make_opt(
					log_level, "log-level",
					"-l", "--log-level",
					"Minimal log level from the list: "
					"(trace, debug, info, warning, error, critical, off), "
					"(default: {})" )
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

		if( restinio_tracing )
			result.m_restinio_tracing = restinio_tracing_t::on;

		if( const auto actual_log_level = log_level_from_str( log_level );
				!actual_log_level )
			throw shrimp::exception_t{
					"Invalid value for log level: {}", log_level };
		else
			result.m_log_level = *actual_log_level;

		result.m_io_threads = io_threads ? io_threads :
				thread_count_from_env_var( "SHRIMP_IO_THREADS" );

		result.m_worker_threads = worker_threads ? worker_threads :
				thread_count_from_env_var( "SHRIMP_WORKER_THREADS" );

		return result;
	}
};

[[nodiscard]]
spdlog::sink_ptr
make_logger_sink()
{
	auto sink = std::make_shared< spdlog::sinks::ansicolor_stdout_sink_mt >();
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
calculate_thread_count(
	const std::optional<thread_count_t> default_io_threads,
	const std::optional<thread_count_t> default_worker_threads )
{
	struct result_t {
		thread_count_t m_io_threads;
		thread_count_t m_worker_threads;
	};

	const auto actual_io_threads_calculator = []() -> thread_count_t {
		constexpr unsigned int max_io_threads = 2u;
		const unsigned int cores = std::thread::hardware_concurrency();
		return { cores < max_io_threads * 3u ? 1u : max_io_threads };
	};

	const auto actual_worker_threads_calculator =
			[](thread_count_t io_threads) -> thread_count_t {
				const unsigned int cores = std::thread::hardware_concurrency();
				return { cores <= io_threads.value() ?
						2u : cores - io_threads.value() };
			};

	const auto io_threads = default_io_threads ?
			*default_io_threads : actual_io_threads_calculator();
	const auto worker_threads = default_worker_threads ?
			*default_worker_threads : actual_worker_threads_calculator(io_threads);

	return result_t{ io_threads, worker_threads };
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
	spdlog::level::level_enum log_level,
	sobj_tracing_t sobj_tracing,
	restinio_tracing_t restinio_tracing,
	const std::optional<thread_count_t> default_io_threads,
	const std::optional<thread_count_t> default_worker_threads )
{
	auto logger_sink = make_logger_sink();
	logger_sink->set_level( log_level );
	
	const auto threads = calculate_thread_count(
			default_io_threads,
			default_worker_threads );
	make_logger( "run_app", logger_sink )->info(
			"shrimp threads count: io_threads={}, worker_threads={}",
			threads.m_io_threads.value(),
			threads.m_worker_threads.value() );

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
							threads.m_worker_threads.value() ) );
		},
		[&]( so_5::environment_params_t & params ) {
			if( sobj_tracing_t::on == sobj_tracing )
				params.message_delivery_tracer(
						spdlog_sobj_tracer_t::make( logger_sink ) );
		}
	};

	// Now we can launch HTTP-server.
	// Logger object is necessary even if logging for RESTinio is turned off.
	auto restinio_logger = make_logger(
			"restinio",
			logger_sink,
			restinio_tracing_t::off == restinio_tracing ?
					spdlog::level::off : log_level );
	// If SObjectizer is not started yet we will wait on the future::get() call.
	restinio::run(
			asio_io_ctx,
			shrimp::make_http_server_settings(
					threads.m_io_threads.value(),
					params,
					std::move(restinio_logger),
					manager_mbox_promise.get_future().get() ) );
}

//
// magick_initializer_t
//
/*!
 * Helper class which uses RAII idiom to initialize and denitialize
 * ImageMagick++ library.
 */
struct magick_initializer_t final {
	magick_initializer_t(const magick_initializer_t &) = delete;
	magick_initializer_t(magick_initializer_t &&) = delete;

	magick_initializer_t(const char * arg)
	{
		Magick::InitializeMagick( arg );
	}

	~magick_initializer_t()
	{
		Magick::TerminateMagick();
	}
};

} /* anonymous namespace */

int
main( int argc, const char * argv[] )
{
	try
	{
		magick_initializer_t magick_init{ *argv };

		const auto args = app_args_t::parse( argc, argv );

		if( !args.m_help )
		{
			run_app(
					args.m_app_params,
					args.m_log_level,
					args.m_sobj_tracing,
					args.m_restinio_tracing,
					args.m_io_threads,
					args.m_worker_threads );
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "ERROR: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

