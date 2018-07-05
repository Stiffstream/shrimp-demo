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
a_transform_manager_t::a_transform_manager_t( context_t ctx )
	: so_5::agent_t{ std::move(ctx) }
{}

void
a_transform_manager_t::so_define_agent()
{
	so_subscribe_self()
			.event( &a_transform_manager_t::on_resize_request )
			.event( &a_transform_manager_t::on_resize_result )
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
	transform::resize_request_key_t request_key{ cmd->m_image, cmd->m_params };

	auto atoken = m_transformed_cache.lookup( request_key );
	if( atoken )
		handle_request_for_already_transformed_image(
				cmd.make_reference(),
				*atoken );
	else
		handle_new_request(
				std::move(request_key),
				cmd.make_reference() );
}

void
a_transform_manager_t::on_resize_result(
	mutable_mhood_t<resize_result_t> cmd )
{
	// Worker now can be stored as free.
	m_free_workers.push( std::move(cmd->m_worker) );

	// Perform actual processing of transformation result.
	std::visit( variant_visitor{
			[&]( successful_resize_t & result ) {
				on_successful_resize(
						result,
						std::move(cmd->m_original_request) );
			},
			[&]( failed_resize_t & result ) {
				on_failed_resize(
						result,
						std::move(cmd->m_original_request) );
			} },
			cmd->m_result );

	try_initiate_pending_requests_processing();
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
	while( !m_pending_requests.empty() &&
			m_pending_requests.front().m_stored_at < time_border )
	{
		pending_request_t request = std::move(m_pending_requests.front());
		m_pending_requests.pop();
		do_504_response( std::move(request.m_cmd->m_http_req) );
	}
}

void
a_transform_manager_t::handle_request_for_already_transformed_image(
	sobj_shptr_t<resize_request_t> cmd,
	cache_t::access_token_t atoken )
{
	// Access time for the cached image should be updated on every access.
	m_transformed_cache.update_access_time( atoken );

	// Form a HTTP-response for that request.
	serve_transformed_image(
			std::move(cmd->m_http_req),
			atoken.value(),
			cmd->m_image_format,
			http_header::image_src_t::cache,
			make_header_fields_list(
					http_header::shrimp_total_processing_time_hf, "0" ) );
}

void
a_transform_manager_t::handle_new_request(
	transform::resize_request_key_t request_key,
	sobj_shptr_t<resize_request_t> cmd )
{
	if( m_pending_requests.size() < max_pending_requests )
	{
		// We have some room to store additional request.
		m_pending_requests.emplace(
				std::move(request_key),
				std::move(cmd),
				std::chrono::steady_clock::now() );

		// If there is a free worker then we can push a request to processing.
		try_initiate_pending_requests_processing();
	}
	else
	{
		// We are overloaded.
		do_503_response( std::move(cmd->m_http_req) );
	}
}

void
a_transform_manager_t::try_initiate_pending_requests_processing()
{
	if( !m_free_workers.empty() )
	{
		// There is a free worker. Let's try to find unique request
		// which isn't in a cache yet.
		while( !m_pending_requests.empty() )
		{
			pending_request_t request = std::move(m_pending_requests.front());
			m_pending_requests.pop();

			// There could be such image in the cache.
			auto atoken = m_transformed_cache.lookup( request.m_key );
			if( atoken )
			{
				handle_request_for_already_transformed_image(
						std::move(request.m_cmd),
						*atoken );
			}
			else
			{
				// There is no transformed image in the cache.
				// Actual image processing should be done.
				auto worker = std::move(m_free_workers.top());
				m_free_workers.pop();
				so_5::send< so_5::mutable_msg<a_transformer_t::resize_request_t> >(
						worker,
						std::move(request.m_cmd),
						so_direct_mbox() );
			}
		}
	}
}

void
a_transform_manager_t::on_successful_resize(
	successful_resize_t & result,
	sobj_shptr_t<resize_request_t> cmd )
{
	store_transformed_image_to_cache(
			transform::resize_request_key_t{ cmd->m_image, cmd->m_params },
			datasizable_blob_shared_ptr_t{ result.m_image_blob } );

	// Transformed image can be sent as response.
	serve_transformed_image(
			std::move(cmd->m_http_req),
			result.m_image_blob,
			cmd->m_image_format,
			http_header::image_src_t::transform,
			make_header_fields_list(
					std::string_view( http_header::shrimp_total_processing_time_hf ),
					fmt::format( "{}",
							// Milliseconds with fractions.
							result.m_duration.count() / 1000.0 ) ) );
}

void
a_transform_manager_t::on_failed_resize(
	failed_resize_t & /*result*/,
	sobj_shptr_t<resize_request_t> cmd )
{
	do_404_response( std::move(cmd->m_http_req) );
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

} /* namespace shrimp */

