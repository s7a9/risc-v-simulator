#pragma once
#ifndef RV_REGISTER_HPP
#define RV_REGISTER_HPP

template <class T>
class Register {
public:
	T cur_data, nxt_data;

	Register(): cur_data(T()), nxt_data(T()) {}

	Register(const T& init_val):
		cur_data(init_val), nxt_data(init_val) {}

	T operator= (const Register& rhs) {
		nxt_data = rhs.cur_data;
		return nxt_data;
	}

	T operator= (T rhs) {
		nxt_data = rhs;
		return rhs;
	}

	void tick() {
		cur_data = nxt_data;
	}

	operator typename T() const {
		return cur_data;
	}
};

#endif // !RV_REGISTER_HPP