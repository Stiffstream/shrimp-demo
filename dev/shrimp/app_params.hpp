/*
 * Shrimp
 */

/*!
 * \file
 * \brief Parameters for various parts of application.
 */

#pragma once

#include <cstdint>
#include <string>

namespace shrimp {

//
// storage_params_t
//

//! Paramaters of image storage.
struct storage_params_t
{
	//! Root directory for original images.
	std::string m_root_dir{ "." };
};

//
// http_server_params_t
//

//! Parameters for HTTP-server.
struct http_server_params_t
{
	enum class ip_version_t : std::uint16_t { v4 = 4, v6 = 6 };

	static constexpr std::uint16_t default_port = 80;
	static constexpr ip_version_t default_ip_version = ip_version_t::v4;
	static constexpr char default_address[] = "localhost";

	// Params directly mapped to RESTinio settings.
	std::uint16_t m_port{ default_port };
	ip_version_t m_ip_version{ default_ip_version };
	std::string m_address{ default_address };
};

//
// app_params_t
//

//! Parameters for the whole application.
struct app_params_t
{
	http_server_params_t m_http_server;

	storage_params_t m_storage;
};

} /* namespace shrimp */

