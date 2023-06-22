#pragma once
#ifndef RV_REGISTER_HPP
#define RV_REGISTER_HPP

template <class T>
class Register {
private:
	T cur_data, nxt_data;

public:
	Register(): cur_data(T()), nxt_data(T()) {}

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