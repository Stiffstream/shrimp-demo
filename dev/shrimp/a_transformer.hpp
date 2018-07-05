/*
 * Shrimp
 */

/*!
 * \file
 * \brief An agent for actual image transformations.
 */

#pragma once

#include <shrimp/a_transform_manager.hpp>
#include <shrimp/app_params.hpp>

namespace shrimp {

//
// a_transformer_t
//
/*!
 * \brief A worker agent which performs actual transformation of an image.
 *
 * This agent receives resize_request_t, performs it and replies by
 * sending a_transform_manager_t::resize_result_t message back.
 */
class a_transformer_t final : public so_5::agent_t
{
public:
	//! A request to be used for new transformation.
	/*!
	 * \note This message should be sent as mutable message.
	 */
	struct resize_request_t final : public so_5::message_t
	{
		//! Original request to be processed.
		sobj_shptr_t<a_transform_manager_t::resize_request_t> m_cmd;
		//! Mbox for the result of the transformation.
		const so_5::mbox_t m_reply_to;

		resize_request_t(
			sobj_shptr_t<a_transform_manager_t::resize_request_t> cmd,
			so_5::mbox_t reply_to )
			: m_cmd{ std::move(cmd) }
			, m_reply_to{ std::move(reply_to) }
		{}
	};

	a_transformer_t( context_t ctx, storage_params_t cfg );

	virtual void
	so_define_agent() override;

private:
	//! A constraint for total count of pixel in resulting image.
	static constexpr std::size_t total_pixel_count{ 5000ul*5000ul };

	//! Configuration for agent.
	const storage_params_t m_cfg;

	void
	on_resize_request(
		mutable_mhood_t<resize_request_t> cmd);

	a_transform_manager_t::resize_result_t::result_t
	handle_resize_request(
		a_transform_manager_t::resize_request_t & request );

	//! Load image from given path.
	[[nodiscard]] Magick::Image
	load_image( std::string_view image_name ) const;
};

} /* namespace shrimp */

