#ifndef VHVC_IO
#define VHVC_IO
#include "common.hh"
#include <vector>
#include "span.hh"
namespace vhvc {
	struct File {
		SDL_RWops* rw;
		int64_t size() {
			return rw->size(rw);
		}
		int64_t seek(int64_t offset, int whence) {
			return rw->seek(rw, offset, whence);
		}
		size_t read(void* ptr, size_t size, size_t maxnum) {
			return rw->read(rw, ptr, size, maxnum);
		}
		size_t write(const void* ptr, size_t size, size_t num) {
			return rw->write(rw, ptr, size, num);
		}
		bool readInto(span<uint8_t> buf) {
			return buf.size() == read(buf.data(), 1, buf.size());
		}
		bool readInto(std::vector<uint8_t>& buf) {
			int64_t sz = size();
			if (sz != -1) {
				buf.resize(sz);
				return readInto(span<uint8_t>(buf.begin(), buf.end()));
			}
			return false;
		}
		File() :rw(nullptr) {}
		explicit File(SDL_RWops* rw) :rw(rw) {
		}
		File(const File&) = delete;
		File& operator=(const File&) = delete;
		File(File&& oth) noexcept {
			rw = oth.rw;
			oth.rw = nullptr;
		}
		File& operator=(File&& oth) noexcept {
			if (rw)
				rw->close(rw);
			rw = oth.rw;
			oth.rw = nullptr;
		}
		~File() {
			if (rw)
				rw->close(rw);
		}
		explicit operator bool() {
			return !!rw;
		}
		static File fromFile(const char* file, const char* mode) {
			return File(SDL_RWFromFile(file, mode));
		}
		static File fromMem(void* mem, int size) {
			return File(SDL_RWFromMem(mem, size));
		}
		static File fromConstMem(const void* mem, int size) {
			return File(SDL_RWFromConstMem(mem, size));
		}
	};
}
#endif
