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
	storage_params_t cfg )
	: so_5::agent_t{ std::move(ctx) }
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
	auto result = handle_resize_request( *(cmd->m_cmd) );

	so_5::send< so_5::mutable_msg<a_transform_manager_t::resize_result_t> >(
			cmd->m_reply_to,
			so_direct_mbox(),
			std::move(cmd->m_cmd),
			std::move(result) );
}

a_transform_manager_t::resize_result_t::result_t
a_transformer_t::handle_resize_request(
	a_transform_manager_t::resize_request_t & request )
{
	try
	{
		auto image = load_image( request.m_image );

		const auto duration = measure_duration_ms( [&]{
				transform::resize(
						request.m_params,
						total_pixel_count,
						image );
			} );

		return a_transform_manager_t::successful_resize_t{
				make_blob( image ),
				std::chrono::duration_cast<std::chrono::microseconds>(duration) };
	}
	catch( const std::exception & x )
	{
		return a_transform_manager_t::failed_resize_t{ x.what() };
	}
}

[[nodiscard]] Magick::Image
a_transformer_t::load_image( std::string_view image_name ) const
{
	Magick::Image image;
	image.read( make_full_path( m_cfg.m_root_dir, image_name ) );

	return image;
}

} /* namespace shrimp */


