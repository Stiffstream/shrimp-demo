/*
 * Shrimp
 */

/*!
 * \file
 * \brief An agent for actual image transformations.
 */

#include <cassert>

#include <shrimp/a_transformer.hpp>

namespace shrimp {

//
// a_transformer_t
//
a_transformer_t::a_transformer_t(
	context_t ctx,
	std::shared_ptr<spdlog::logger> logger,
	storage_params_t cfg )
	: so_5::agent_t{ std::move(ctx) }
	, m_logger{ std::move(logger) }
	, m_cfg{ std::move(cfg) }
{}

void
a_transformer_t::so_define_agent()
{
	so_subscribe_self().event(
			&a_transformer_t::on_resize_request );
}

void
a_transformer_t::on_resize_request(
	mutable_mhood_t<resize_request_t> cmd)
{
	auto result = handle_resize_request( cmd->m_key );

	so_5::send< so_5::mutable_msg<a_transform_manager_t::resize_result_t> >(
			cmd->m_reply_to,
			so_direct_mbox(),
			std::move(cmd->m_key),
			std::move(result) );
}

[[nodiscard]]
a_transform_manager_t::resize_result_t::result_t
a_transformer_t::handle_resize_request(
	const transform::resize_request_key_t & key )
{
	try
	{
		m_logger->trace( "transformation started; request_key={}", key );

		auto image = load_image( key.path() );

		const auto resize_duration = measure_duration( [&]{
				transform::resize(
						key.params(),
						total_pixel_count,
						image );
			} );
		m_logger->debug( "resize finished; request_key={}, time={}ms",
				key,
				std::chrono::duration_cast<std::chrono::milliseconds>(
						resize_duration).count() );

		datasizable_blob_shared_ptr_t blob;
		const auto serialize_duration = measure_duration( [&] {
					blob = make_blob( image );
				} );
		m_logger->debug( "serialization finished; request_key={}, time={}ms",
				key,
				std::chrono::duration_cast<std::chrono::milliseconds>(
						serialize_duration).count() );

		return a_transform_manager_t::successful_resize_t{
				std::move(blob),
				std::chrono::duration_cast<std::chrono::microseconds>(
						resize_duration + serialize_duration) };
	}
	catch( const std::exception & x )
	{
		return a_transform_manager_t::failed_resize_t{ x.what() };
	}
}

[[nodiscard]]
Magick::Image
a_transformer_t::load_image( std::string_view image_name ) const
{
	Magick::Image image;
	image.read( make_full_path( m_cfg.m_root_dir, image_name ) );

	return image;
}

} /* namespace shrimp */


