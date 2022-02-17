#ifndef VHVC_SPAN
#define VHVC_SPAN
#include <stdint.h>
namespace vhvc {
	template<typename T>
	class span {
		T* m_data;
		size_t m_size;
	public:
		constexpr span() noexcept :m_data(nullptr), m_size(0) {}
		template<typename It>
		constexpr span(It data, size_t size) noexcept :m_data(&*data), m_size(size) {}
		template<typename It>
		constexpr span(It begin, It end) noexcept :m_data(&*begin), m_size(end - begin) {}
		template<size_t N>
		constexpr span(T(&arr)[N]) noexcept :m_data(arr), m_size(N) {}
		template<typename R>
		constexpr span(R&& source) : m_data(source.data()), m_size(source.size()) {}
		template<typename U>
		constexpr span(const span<U>& source) noexcept :m_data(source.data()), m_size(source.size()) {}
		constexpr T& operator[](size_t idx) const { return m_data[idx]; }
		constexpr T* data() const noexcept { return m_data; }
		constexpr size_t size() const noexcept { return m_size; }
		constexpr bool empty() const noexcept { return !m_size; }
		constexpr span<T> first(size_t count) const {
			return span<T>(m_data, count);
		}
		constexpr span<T> last(size_t count) const {
			return span<T>(m_data + m_size - count, count);
		}
		constexpr span<T> subspan(size_t offset, size_t count = -1) const {
			return span<T>(m_data + offset, count == -1 ? count - offset : count);
		}
	};
	template<typename T>
	using cspan = span<const T>;
	using span_u8 = span<uint8_t>;
	using cspan_u8 = cspan<uint8_t>;
}
#endif
