/*
 * Shrimp
 */

/*!
 * \file
 * \brief A manager for transform operations.
 */

#pragma once

#include <shrimp/transforms.hpp>
#include <shrimp/cache_alike_container.hpp>

#include <so_5/all.hpp>
#include <restinio/all.hpp>

#include <queue>
#include <stack>
#include <variant>

namespace shrimp {

/*!
 * \brief The transformation manager agent.
 *
 * This agent holds a cache of transformed images. If an image is already
 * present in that cache the response is sent immediatelly.
 *
 * If there is no image in the cache the request will be added to a queue
 * of pending requests. When a free transformer (worker) agent become
 * available a pending request will be scheduled to that free worker.
 *
 * This agent receives results from workers and produces responses to
 * original requests.
 *
 * This agent periodically checks queue of pending requests. If a request is
 * in the queue for a long time then this request will be removed and
 * negative response will be sent to the original request.
 *
 * This agent periodically checks cache's contents and removes too old
 * images from it.
 */
class a_transform_manager_t final : public so_5::agent_t
{
public:
	//! A request to be used for new transformation.
	/*!
	 * \note This message should be sent as mutable message.
	 */
	struct resize_request_t final : public so_5::message_t
	{
		//! Original HTTP-request.
		restinio::request_handle_t m_http_req;
		//! Image name.
		std::string m_image;
		//! Type of image to be transformed.
		image_format_t m_image_format;
		//! Transformation parameters.
		transform::resize_params_t m_params;

		resize_request_t(
			restinio::request_handle_t http_req,
			std::string image,
			image_format_t image_format,
			transform::resize_params_t params )
			: m_http_req{ std::move(http_req) }
			, m_image{ std::move(image) }
			, m_image_format{ image_format }
			, m_params{ params }
		{}
	};

	//! Description of successful transformation result.
	struct successful_resize_t
	{
		//! Transformed image in form of BLOB.
		datasizable_blob_shared_ptr_t m_image_blob;
		//! Time spent on this transformation.
		std::chrono::microseconds m_duration;
	};

	//! Description of failed transformation result.
	struct failed_resize_t
	{
		//! Textual description of a failure.
		std::string m_reason;
	};

	//! Message with result of image transformation.
	/*!
	 * \note This message should be sent as mutable message.
	 */
	struct resize_result_t final : public so_5::message_t
	{
		using result_t = std::variant<successful_resize_t, failed_resize_t>;

		//! Who processed the request.
		so_5::mbox_t m_worker;
		//! Original transformation request.
		sobj_shptr_t<resize_request_t> m_original_request;

		//! The result of the transformation.
		result_t m_result;

		template<typename Actual_Result>
		resize_result_t(
			so_5::mbox_t worker,
			sobj_shptr_t<resize_request_t> original_request,
			Actual_Result result)
			: m_worker{ std::move(worker) }
			, m_original_request{ std::move(original_request) }
			, m_result{ std::move(result) }
		{}
	};

	a_transform_manager_t( context_t ctx );

	virtual void
	so_define_agent() override;

	virtual void
	so_evt_start() override;

	//! Add a mbox of another worker agent.
	/*!
	 * This method must be called before the registration of
	 * cooperation with transformers and transformer manager agents.
	 */
	void
	add_worker( so_5::mbox_t worker );

private :
	//! Type of container for cache of processed images.
	using cache_t = cache_alike_container_t<
			transform::resize_request_key_t,
			datasizable_blob_shared_ptr_t >;

	//! Description of pending requests.
	struct pending_request_t
	{
		//! Key for that request.
		transform::resize_request_key_t m_key;
		//! Original request.
		sobj_shptr_t<resize_request_t> m_cmd;
		//! When request was received.
		std::chrono::steady_clock::time_point m_stored_at;

		pending_request_t(
			transform::resize_request_key_t key,
			sobj_shptr_t<resize_request_t> cmd,
			std::chrono::steady_clock::time_point stored_at )
			: m_key{ std::move(key) }
			, m_cmd{ std::move(cmd) }
			, m_stored_at{ stored_at }
		{}
	};

	//! Type of queue of pending requests.
	using pending_request_queue_t = std::queue<pending_request_t>;

	//! Type of container with free workers.
	using free_worker_container_t = std::stack<so_5::mbox_t>;

	//! A special signal to remove oldest images from the cache.
	struct clear_cache_t final : public so_5::signal_t {};

	//! A special signal to check lifetime of pending requests.
	struct check_pending_requests_t final : public so_5::signal_t {};

	//! Cache of processed images.
	cache_t m_transformed_cache;

	//! Total amount of memory occuped by processed images.
	std::uint_fast64_t m_transformed_cache_memory_size{ 0u };
	//! Max size of cache of transformed images.
	static constexpr std::uint_fast64_t max_transformed_cache_memory_size{
			100ul * 1024ul * 1024ul };

	//! Queue of pending requests.
	pending_request_queue_t m_pending_requests;
	//! Max count of pending requests.
	static constexpr std::size_t max_pending_requests{ 64u };

	//! Container of free workers.
	free_worker_container_t m_free_workers;

	//! Timer for clear_cache operation.
	so_5::timer_id_t m_clear_cache_timer;
	//! Interval for clear cache operations.
	static constexpr std::chrono::minutes clear_cache_period{ 1 };
	//! Max time of storing an image in the cache.
	static constexpr std::chrono::hours max_cache_lifetime{ 1 };

	//! Timer for checking pending requests.
	so_5::timer_id_t m_check_pending_timer;
	//! Interval for check pending requests.
	static constexpr std::chrono::seconds check_pending_period{ 5 };
	//! Max time for waiting of pending request.
	static constexpr std::chrono::seconds max_pending_time{ 20 };

	void
	on_resize_request(
		mutable_mhood_t<resize_request_t> cmd );

	void
	on_resize_result(
		mutable_mhood_t<resize_result_t> cmd );

	void
	on_clear_cache(
		mhood_t<clear_cache_t> );

	void
	on_check_pending_requests(
		mhood_t<check_pending_requests_t> );

	void
	handle_request_for_already_transformed_image(
		sobj_shptr_t<resize_request_t> cmd,
		cache_t::access_token_t atoken );

	void
	handle_new_request(
		transform::resize_request_key_t key,
		sobj_shptr_t<resize_request_t> cmd );

	void
	try_initiate_pending_requests_processing();

	void
	on_successful_resize(
		successful_resize_t & result,
		sobj_shptr_t<resize_request_t> cmd );

	void
	on_failed_resize(
		failed_resize_t & result,
		sobj_shptr_t<resize_request_t> cmd );

	void
	store_transformed_image_to_cache(
		transform::resize_request_key_t key,
		datasizable_blob_shared_ptr_t image_blob );
};

} /* namespace shrimp */

