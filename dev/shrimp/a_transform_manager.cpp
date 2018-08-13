/*
 * Shrimp
 */

/*!
 * \file
 * \brief A manager for transform operations.
 */

#include <cassert>

#include <shrimp/a_transform_manager.hpp>

#include <shrimp/response_common.hpp>
#include <shrimp/utils.hpp>
#include <shrimp/a_transformer.hpp>

namespace shrimp {

//
// a_transform_manager_t
//
a_transform_manager_t::a_transform_manager_t(
	context_t ctx,
	std::shared_ptr<spdlog::logger> logger )
	: so_5::agent_t{ std::move(ctx) }
	, m_logger{ std::move(logger) }
{}

void
a_transform_manager_t::so_define_agent()
{
	so_subscribe_self()
			.event( &a_transform_manager_t::on_resize_request )
			.event( &a_transform_manager_t::on_resize_result )
			.event( &a_transform_manager_t::on_delete_cache_request )
			.event( &a_transform_manager_t::on_negative_delete_cache_response )
			.event( &a_transform_manager_t::on_clear_cache )
			.event( &a_transform_manager_t::on_check_pending_requests );
}

void
a_transform_manager_t::so_evt_start()
{
	// Periodic signal for clearing cache from oldest messages
	// must be started.
	m_clear_cache_timer = so_5::send_periodic<clear_cache_t>(
			*this,
			clear_cache_period,
			clear_cache_period );

	// Periodic signal for checking pending requests must be started.
	m_check_pending_timer = so_5::send_periodic<check_pending_requests_t>(
			*this,
			check_pending_period,
			check_pending_period );
}

void
a_transform_manager_t::add_worker( so_5::mbox_t worker )
{
	m_free_workers.push( std::move(worker) );
}

void
a_transform_manager_t::on_resize_request(
	mutable_mhood_t<resize_request_t> cmd )
{
	transform::resize_request_key_t request_key{
			cmd->m_image,
			cmd->m_target_format,
			cmd->m_params };

	m_logger->trace( "request received; request_key={}, connection_id={}",
			request_key,
			cmd->m_http_req->connection_id() );

	auto atoken = m_transformed_cache.lookup( request_key );
	if( atoken )
		handle_request_for_already_transformed_image(
				cmd.make_reference(),
				*atoken );
	else
		handle_not_transformed_image(
				std::move(request_key),
				cmd.make_reference() );
}

void
a_transform_manager_t::on_resize_result(
	mutable_mhood_t<resize_result_t> cmd )
{
	m_logger->trace( "resize_result received; request_key={}, worker_mbox={}",
			cmd->m_key,
			cmd->m_worker->id() );

	// Worker now can be stored as free and processing of
	// some pending request can be initiated.
	m_free_workers.push( std::move(cmd->m_worker) );
	try_initiate_pending_requests_processing();

	// Extract all related to this image information from in-progress queue.
	auto key = std::move(cmd->m_key);
	auto requests = extract_inprogress_requests(
			std::move(m_inprogress_requests.find_first_for_key( key ).value()) );

	// Perform actual processing of transformation result.
	std::visit( variant_visitor{
			[&]( successful_resize_t & result ) {
				on_successful_resize(
						std::move(key),
						result,
						std::move(requests) );
			},
			[&]( failed_resize_t & result ) {
				on_failed_resize(
						std::move(key),
						result,
						std::move(requests) );
			} },
			cmd->m_result );
}

void
a_transform_manager_t::on_delete_cache_request(
	mutable_mhood_t<delete_cache_request_t> cmd )
{
	m_logger->warn( "delete cache request received; "
			"connection_id={}, token={}",
			cmd->m_http_req->connection_id(),
			cmd->m_token );

	const auto delay_response = [&]( std::string response_text ) {
		so_5::send_delayed< so_5::mutable_msg<negative_delete_cache_response_t> >(
				*this,
				std::chrono::seconds{7},
				std::move(cmd->m_http_req),
				std::move(response_text) );
	};

	if( const char * env_token = std::getenv( "SHRIMP_ADMIN_TOKEN" );
			// Token must be present and must not be empty.
			env_token && *env_token )
	{
		if( cmd->m_token == env_token )
		{
			m_transformed_cache.clear();

			m_logger->info( "cache deleted" );

			do_200_plaintext_response(
					std::move(cmd->m_http_req),
					"Cache deleted\r\n" );
		}
		else
		{
			m_logger->error( "invalid token value for delete cache request; "
					"token={}",
					cmd->m_token );

			delay_response( "Token value mismatch\r\n" );
		}
	}
	else
	{
		m_logger->warn( "delete cache can't be performed because there is no "
				"admin token defined" );

		// Operation can't be performed because admin token is not avaliable.
		delay_response( "No admin token defined\r\n" );
	}
}

void
a_transform_manager_t::on_negative_delete_cache_response(
	mutable_mhood_t<negative_delete_cache_response_t> cmd )
{
	m_logger->debug( "send negative response to delete cache request; "
			"connection_id={}",
			cmd->m_http_req->connection_id() );

	do_403_response(
			std::move(cmd->m_http_req),
			std::move(cmd->m_response_text) );
}

void
a_transform_manager_t::on_clear_cache(
	mhood_t<clear_cache_t> )
{
	const auto time_border = std::chrono::steady_clock::now() - max_cache_lifetime;
	while( !m_transformed_cache.empty() )
	{
		auto atoken = m_transformed_cache.oldest().value();
		if( atoken.access_time() < time_border )
		{
			// This image is too old and should be removed.
			m_transformed_cache.erase( atoken );
		}
		else
			// Clearance procedure can be stopped because this and all other
			// images are too young to be removed from cache.
			break;
	}
}

void
a_transform_manager_t::on_check_pending_requests(
	mhood_t<check_pending_requests_t> )
{
	const auto time_border = std::chrono::steady_clock::now() - max_pending_time;

	while( !m_pending_requests.empty() )
	{
		auto atoken = m_pending_requests.oldest().value();
		if( atoken.access_time() < time_border )
		{
			// This request should be removed.
			auto http_req = std::move(atoken.value()->m_http_req);

			m_logger->warn( "reject pending request, too long waiting time; "
					"request_key={}, connection_id={}",
					atoken.key(),
					http_req->connection_id() );

			m_pending_requests.erase( std::move(atoken) );
			do_504_response( std::move(http_req) );
		}
		else
			break;
	}
}

void
a_transform_manager_t::handle_request_for_already_transformed_image(
	sobj_shptr_t<resize_request_t> cmd,
	cache_t::access_token_t atoken )
{
	m_logger->debug( "transformed image is present in cache; request_key={}",
			atoken.key() );

	// Access time for the cached image should be updated on every access.
	m_transformed_cache.update_access_time( atoken );

	// Form a HTTP-response for that request.
	serve_transformed_image(
			std::move(cmd->m_http_req),
			atoken.value(),
			cmd->m_target_format,
			http_header::image_src_t::cache,
			make_header_fields_list(
					http_header::shrimp_total_processing_time_hf(), "0" ) );
}

void
a_transform_manager_t::handle_not_transformed_image(
	transform::resize_request_key_t request_key,
	sobj_shptr_t<resize_request_t> cmd )
{
	const auto store_to = [&](auto & queue) {
		queue.insert( std::move(request_key), std::move(cmd) );
	};

	if( m_inprogress_requests.has_key( request_key ) )
	{
		// Same request is already in progress.
		m_logger->debug( "same request is already in progress; request_key={}",
				request_key );

		// New request must be stored as in-progress request.
		store_to( m_inprogress_requests );
	}
	else if( m_pending_requests.has_key( request_key ) )
	{
		// Same request is already pending for free worker.
		m_logger->debug( "same request is already pending; request_key={}",
				request_key );

		store_to( m_pending_requests );
	}
	else if( m_pending_requests.unique_keys() < max_pending_requests )
	{
		// This is a new request and we can store it as pending request.
		m_logger->debug( "store request to pending requests queue; request_key={}",
				request_key );

		store_to( m_pending_requests );

		// If there is a free worker then we can push a request to processing.
		try_initiate_pending_requests_processing();
	}
	else 
	{
		// We are overloaded.
		m_logger->warn( "request is rejected because of overloading; "
				"request_key={}",
				request_key );

		do_503_response( std::move(cmd->m_http_req) );
	}
}

void
a_transform_manager_t::try_initiate_pending_requests_processing()
{
	// Let's try to find unique request which isn't in a cache yet.
	// But do the search only if there is at least one free worker.
	while( !m_free_workers.empty() && !m_pending_requests.empty() )
	{
		auto atoken = m_pending_requests.oldest().value();
		const auto key = atoken.key();

		// We can't restore is an exception will be thrown during
		// requests movement.
		[&]() noexcept {
			m_pending_requests.extract_values_for_key(
					std::move(atoken),
					[&]( auto && value ) {
						m_inprogress_requests.insert(
								// insert() expect an rvalue, because of that
								// we should create a copy of the key.
								transform::resize_request_key_t{key},
								std::move(value) );
					} );
		}();

		// Allocate a worker for that image.
		auto worker = std::move(m_free_workers.top());
		m_free_workers.pop();

		m_logger->trace( "initiate processing of a request; "
				"request_key={}, worker_mbox={}",
				key, worker->id() );

		so_5::send< so_5::mutable_msg<a_transformer_t::resize_request_t> >(
				worker,
				key,
				so_direct_mbox() );
	}
}

void
a_transform_manager_t::on_successful_resize(
	transform::resize_request_key_t key,
	successful_resize_t & result,
	original_request_container_t requests )
{
	m_logger->debug( "successul resize result; request_key={}, blob_size={}",
			key,
			result.m_image_blob->size() );

	store_transformed_image_to_cache(
			transform::resize_request_key_t{ key },
			datasizable_blob_shared_ptr_t{ result.m_image_blob } );

	// Milliseconds with fractions from microseconds.
	const auto us_to_ms = [](auto us) { return us.count() / 1000.0; };

	// Additional headers for every response.
	auto additional_headers = make_header_fields_list(
			http_header::shrimp_total_processing_time_hf(),
			fmt::format( "{}",
					us_to_ms(result.m_resize_duration + result.m_encoding_duration) ),
			http_header::shrimp_resize_time_hf(),
			fmt::format( "{}", us_to_ms(result.m_resize_duration) ),
			http_header::shrimp_encoding_time_hf(),
			fmt::format( "{}", us_to_ms(result.m_encoding_duration) ) );

	for( auto & rq : requests )
	{
		m_logger->trace( "sending positive response back; "
				"request_key={}, connection_id={}",
				key,
				rq->m_http_req->connection_id() );

		// Transformed image can be sent as response.
		serve_transformed_image(
				std::move(rq->m_http_req),
				result.m_image_blob,
				rq->m_target_format,
				http_header::image_src_t::transform,
				additional_headers );
	}
}

void
a_transform_manager_t::on_failed_resize(
	transform::resize_request_key_t key,
	failed_resize_t & result,
	original_request_container_t requests )
{
	m_logger->warn( "failed resize; request_key={}, reason={}",
			key,
			result.m_reason );

	for( auto & rq : requests )
	{
		m_logger->trace( "sending negative response back; "
				"request_key={}, connection_id={}",
				key,
				rq->m_http_req->connection_id() );

		do_404_response( std::move(rq->m_http_req) );
	}
}

void
a_transform_manager_t::store_transformed_image_to_cache(
	transform::resize_request_key_t key,
	datasizable_blob_shared_ptr_t image_blob )
{
	// Precalculate the new size of a cache.
	const auto updated_cache_size = m_transformed_cache_memory_size +
			image_blob->size();

	// Move transformed image into cache.
	m_transformed_cache.insert( std::move(key), std::move(image_blob) );
	m_transformed_cache_memory_size = updated_cache_size;

	// Cache can exceed it max size. Some old images must be removed
	// in that case. But at least one image should stay inside the cache.
	while(
		max_transformed_cache_memory_size < m_transformed_cache_memory_size &&
		1 < m_transformed_cache.size() )
	{
		auto atoken = m_transformed_cache.oldest().value();
		m_transformed_cache_memory_size -= atoken.value()->size();
		m_transformed_cache.erase( atoken );
	}
}

[[nodiscard]]
a_transform_manager_t::original_request_container_t
a_transform_manager_t::extract_inprogress_requests(
	pending_request_queue_t::access_token_t atoken )
{
	original_request_container_t result;

	m_inprogress_requests.extract_values_for_key(
			std::move(atoken),
			[&result]( auto && v ) {
				result.emplace_back( std::move(v) );
			} );

	return result;
}

} /* namespace shrimp */

