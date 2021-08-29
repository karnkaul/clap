#pragma once
#include <array>
#include <iterator>
#include <type_traits>
#include <vector>

namespace clap {
///
/// \brief Wrapper around char/int key for an option
///
struct option_key {
	int key{};

	///
	/// \brief Construct via char literal
	///
	constexpr option_key(char key) noexcept : key(key) {}
	///
	/// \brief Construct via integer
	///
	constexpr option_key(int key) noexcept : key(key) {}

	///
	/// \brief Check if value is alpha (char key)
	///
	constexpr bool is_alpha() const noexcept { return (key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z'); }
	///
	/// \brief Obtain key as char
	///
	constexpr char alpha() const noexcept { return static_cast<char>(key); }
	///
	/// \brief Convert to int
	///
	constexpr operator int() const noexcept { return key; }
};

///
/// \brief Bidirectionally iteratable view into a contiguous array ot const Ts / a single const T
///
/// Construction supported from: single value, C array, std::array, std::vector, random_access_iterator pair, pointer to first + extent (explicit)
///
template <typename T>
class array_view {
	static_assert(!std::is_reference_v<T>);
	template <typename It>
	using iter_cat_t = typename std::iterator_traits<It>::iterator_category;
	template <typename It>
	static constexpr bool random_iter = std::is_same_v<iter_cat_t<It>, std::random_access_iterator_tag>;
	template <typename It>
	using enable_if_ra = std::enable_if_t<random_iter<It>>;

  public:
	using value_type = T;
	using const_iterator = T const*;
	using iterator = const_iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;

	constexpr explicit array_view(T const* first = {}, std::size_t extent = 0) noexcept : m_zero(first), m_extent(extent) {}
	constexpr array_view(T const& t) noexcept : array_view(&t, 1) {}
	template <std::size_t N>
	constexpr array_view(T const (&array)[N]) noexcept : array_view(array, N) {}
	template <std::size_t N>
	constexpr array_view(std::array<T, N> const& array) noexcept : array_view(array.data(), array.size()) {}
	array_view(std::vector<T> const& vector) noexcept : array_view(vector.data(), vector.size()) {}
	template <typename It, typename = enable_if_ra<It>>
	constexpr array_view(It first, It last) noexcept : array_view(last == first ? nullptr : &*first, last - first) {}

	constexpr bool empty() const noexcept { return m_extent == 0; }
	constexpr std::size_t size() const noexcept { return m_extent; }

	constexpr const_iterator begin() const noexcept { return m_zero; }
	constexpr const_iterator end() const noexcept { return m_zero + m_extent; }
	constexpr const_iterator cbegin() const noexcept { return m_zero; }
	constexpr const_iterator cend() const noexcept { return m_zero + m_extent; }
	constexpr const_iterator rbegin() const noexcept { return reverse_iterator(end()); }
	constexpr const_iterator rend() const noexcept { return reverse_iterator(begin()); }

  private:
	T const* m_zero;
	std::size_t m_extent;
};
} // namespace clap
