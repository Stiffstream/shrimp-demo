/*
 * Shrimp
 */

/*!
 * \file
 * \brief A template of container which can be used as cache.
 */

#pragma once

#include <map>
#include <list>
#include <chrono>
#include <optional>

namespace shrimp {

/*
 * A container with intended to be used as cache for some values accessible
 * by some keys.
 *
 * All keys should be unique. An attempt to insert a new value for already
 * known key will be ignored sliently.
 *
 * When a new value is stored into container a timestamp is also stored.
 * All values are ordered by timestamps. Oldest value can be accessed by
 * oldest() method.
 *
 * Timestamp for a value can be updated manually by update_access_time()
 * method.
 */
template<typename Key, typename Value>
class cache_alike_container_t
{
	using steady_clock = std::chrono::steady_clock;
	using timepoint_t = steady_clock::time_point;

	struct wrapped_value_t;

	using map_t = std::map<Key, wrapped_value_t>;

	struct access_info_t
	{
		// When the value was accessed last time.
		timepoint_t m_access_time;

		// A reference to the value in form of iterator.
		typename map_t::iterator m_value_it;

		access_info_t() {}
		access_info_t( timepoint_t tp ) : m_access_time{tp} {}
	};

	using access_info_container_t = std::list<access_info_t>;

	// Type of wrapped value to be stored inside an associative container.
	struct wrapped_value_t
	{
		// Actual value.
		Value m_value;

		// A reference to the info about last access time in form of iterator.
		typename access_info_container_t::iterator m_access_it;

		// Initializing constructor.
		wrapped_value_t(
			Value && v,
			typename access_info_container_t::iterator access_it )
			: m_value{ std::move(v) }
			, m_access_it{ std::move(access_it) }
		{}
	};

	map_t m_items;
	access_info_container_t m_access_info;

public :
	class access_token_t 
	{
		template<typename, typename> friend class cache_alike_container_t;

		typename map_t::iterator m_it;

		access_token_t( typename map_t::iterator it ) : m_it{ std::move(it) } {}

		[[nodiscard]] auto
		access_it() const noexcept { return m_it->second.m_access_it; }

		[[nodiscard]] auto
		value_it() const noexcept { return m_it; }

	public:
		[[nodiscard]] const Key &
		key() const noexcept { return m_it->first; }

		[[nodiscard]] Value &
		value() noexcept { return m_it->second.m_value; }

		[[nodiscard]] const Value &
		value() const noexcept { return m_it->second.m_value; }

		[[nodiscard]] auto
		access_time() const noexcept { return access_it()->m_access_time; }
	};

	// This is Moveable type, not Copyable nor Copyconstructible.
	cache_alike_container_t( const cache_alike_container_t & ) = delete;
	cache_alike_container_t & operator=(
			const cache_alike_container_t & ) = delete;

	cache_alike_container_t( cache_alike_container_t && ) = default;
	cache_alike_container_t & operator=(
			cache_alike_container_t && ) = default;

	cache_alike_container_t() = default;

	void
	insert( Key && key, Value && value )
	{
		// For exception safety we use a temporary access_info's list.
		// Content of this list will be then moved into m_access_info.
		access_info_container_t tmp_ac{ { steady_clock::now() } };

		if( auto [it, inserted] = m_items.emplace(
				std::move(key), 
				wrapped_value_t{ std::move(value), tmp_ac.begin() } );
			inserted )
		{
			// It was an unique value which was successfully inserted.
			// Now we can store access info for it.
			tmp_ac.front().m_value_it = it;
			// Move the all content of temporary list to m_access_info.
			m_access_info.splice( m_access_info.end(), tmp_ac, tmp_ac.begin() );
		}
	}

	// Note: this method is not const because an user can change value
	// via obtained access_token.
	[[nodiscard]] std::optional< access_token_t >
	lookup( const Key & key ) noexcept
	{
		if( auto it = m_items.find( key ); it != m_items.end() )
			return access_token_t{ it };
		else
			return std::nullopt;
	}

	void
	erase( access_token_t atoken ) noexcept
	{
		// Access info must be removed first.
		m_access_info.erase( atoken.access_it() );
		// Now the actual value can be removed.
		m_items.erase( atoken.value_it() );
	}

	[[nodiscard]] bool
	empty() const noexcept
	{
		return m_items.empty();
	}

	[[nodiscard]] auto
	size() const noexcept
	{
		return m_items.size();
	}

	[[nodiscard]] std::optional< access_token_t >
	oldest() noexcept
	{
		if( !empty() )
			return access_token_t{ m_access_info.begin()->m_value_it };
		else
			return std::nullopt;
	}

	void
	update_access_time( access_token_t atoken )
		noexcept(noexcept(steady_clock::now()))
	{
		auto it = atoken.access_it();
		it->m_access_time = steady_clock::now();

		// Use splice operation in order not to allocate.
		m_access_info.splice( m_access_info.end(), m_access_info, it );
	}
};

//FIXME: document this!
template<typename Key, typename Value>
class multivalue_cache_alike_container_t
{
	using steady_clock = std::chrono::steady_clock;
	using timepoint_t = steady_clock::time_point;

	struct wrapped_value_t;

	using map_t = std::multimap<Key, wrapped_value_t>;

	struct access_info_t
	{
		// When the value was accessed last time.
		timepoint_t m_access_time;

		// A reference to the value in form of iterator.
		typename map_t::iterator m_value_it;

		access_info_t() {}
		access_info_t( timepoint_t tp ) : m_access_time{tp} {}
	};

	using access_info_container_t = std::list<access_info_t>;

	// Type of wrapped value to be stored inside an associative container.
	struct wrapped_value_t
	{
		// Actual value.
		Value m_value;

		// A reference to the info about last access time in form of iterator.
		typename access_info_container_t::iterator m_access_it;

		// Initializing constructor.
		wrapped_value_t(
			Value && v,
			typename access_info_container_t::iterator access_it )
			: m_value{ std::move(v) }
			, m_access_it{ std::move(access_it) }
		{}
	};

	map_t m_items;
	access_info_container_t m_access_info;
	std::size_t m_unique_keys{};

public :
	class access_token_t 
	{
		template<typename, typename> friend class multivalue_cache_alike_container_t;

		typename map_t::iterator m_it;

		access_token_t( typename map_t::iterator it ) : m_it{ std::move(it) } {}

		[[nodiscard]] auto
		access_it() const noexcept { return m_it->second.m_access_it; }

		[[nodiscard]] auto
		value_it() const noexcept { return m_it; }

	public:
		[[nodiscard]] const Key &
		key() const noexcept { return m_it->first; }

		[[nodiscard]] Value &
		value() noexcept { return m_it->second.m_value; }

		[[nodiscard]] const Value &
		value() const noexcept { return m_it->second.m_value; }

		[[nodiscard]] auto
		access_time() const noexcept { return access_it()->m_access_time; }
	};

	// This is Moveable type, not Copyable nor Copyconstructible.
	multivalue_cache_alike_container_t(
			const multivalue_cache_alike_container_t & ) = delete;
	multivalue_cache_alike_container_t & operator=(
			const multivalue_cache_alike_container_t & ) = delete;

	multivalue_cache_alike_container_t(
			multivalue_cache_alike_container_t && ) = default;
	multivalue_cache_alike_container_t & operator=(
			multivalue_cache_alike_container_t && ) = default;

	multivalue_cache_alike_container_t() = default;

	void
	insert( Key && key, Value && value )
	{
		// For exception safety we use a temporary access_info's list.
		// Content of this list will be then moved into m_access_info.
		access_info_container_t tmp_ac{ { steady_clock::now() } };
		auto it = m_items.emplace( std::move(key),
				wrapped_value_t{ std::move(value), tmp_ac.begin() } );

		// Now we can store access info for it.
		tmp_ac.front().m_value_it = it;
		// Move the all content of temporary list to m_access_info.
		m_access_info.splice( m_access_info.end(), tmp_ac, tmp_ac.begin() );

		// Count of unique keys must be updated if it was a new key.
		const auto is_unique = [&] {
			return !(it != m_items.begin() &&
				std::prev(it)->first == it->first);
		};
		m_unique_keys += is_unique() ? 1u : 0u;
	}

	[[nodiscard]] bool
	has_key( const Key & key ) const noexcept
	{
		return m_items.find( key ) != m_items.end();
	}

	void
	erase( access_token_t atoken ) noexcept
	{
		// Access info must be removed first.
		m_access_info.erase( atoken.access_it() );

		// Now the actual value can be removed.
		// But count of unique keys must be updated first.
		const auto is_unique = [&] {
			const auto it = atoken.value_it();
			if( it != m_items.begin() )
			{
				if( std::prev(it)->first == it->first )
					return false;
			}
			const auto next = std::next(it);
			if( next != m_items.end() )
			{
				if( next->first == it->first )
					return false;
			}

			return true;
		};
		m_unique_keys -= is_unique() ? 1u : 0u;

		// Remove the actual value.
		m_items.erase( atoken.value_it() );
	}

	[[nodiscard]] bool
	empty() const noexcept
	{
		return m_items.empty();
	}

	[[nodiscard]] auto
	unique_keys() const noexcept
	{
		return m_unique_keys;
	}

	[[nodiscard]] std::optional< access_token_t >
	oldest() noexcept
	{
		if( !empty() )
			return access_token_t{ m_access_info.begin()->m_value_it };
		else
			return std::nullopt;
	}

	template<typename L>
	void
	extract_values_for_key_of(
		access_token_t atoken,
		L && lambda )
	{
		const auto key = atoken.key();

		// Move range of values from the main storage into the temporary buffer.
		map_t tmp_storage;
		// Work can't be continued if move operation throws.
		const auto move_to_tmp = [&]() noexcept {
			auto range = m_items.equal_range( key );
			while( range.first != range.second )
			{
				auto it = range.first++;
				m_access_info.erase( it->second.m_access_it );
				tmp_storage.insert( m_items.extract(it) );
			}
		};
		move_to_tmp();

		// Count of unique keys can be safely incremented by 1.
		--m_unique_keys;

		// Move the extracted values to user's lambda.
		for( auto & [k, v] : tmp_storage )
			lambda( std::move(v.m_value) );
	}
};

} /* namespace shrimp */

