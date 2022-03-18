#ifndef VHVC_STATE
#define VHVC_STATE
#include "common.hh"
namespace vhvc {
	struct IState {
		virtual void label(const char *text) =0;
		virtual void use(uint8_t& data) =0;
		virtual void use(uint16_t& data) =0;
		virtual void use(uint32_t& data) =0;
		virtual void use(bool& data) =0;
		virtual void use(uint8_t* data, size_t n) =0;
	};
	inline IState &operator<(IState& st, const char *text) {
		st.label(text);
		return st;
	}
	template<typename T>
	IState &operator>(IState& st, T& data) {
		st.use(data);
		return st;
	}
	template<typename T, size_t N>
	IState &operator>(IState& st, T (&data)[N]) {
		st.use(data, N);
		return st;
	}
	struct LogState : IState {
		void label(const char *text) {
			SDL_Log("%s", text);
		}
		void use(uint8_t& data) {
			SDL_Log("%02X", data);
		}
		void use(uint16_t& data) {
			SDL_Log("%04X", data);
		}
		void use(uint32_t& data) {
			SDL_Log("%08X", data);
		}
		void use(bool& data) {
			SDL_Log(data ? "true" : "false");
		}
		void use(uint8_t* data, size_t n) {
			(void)data;
			SDL_Log("<array of %zu>", n);
		}
	};
}
#endif
