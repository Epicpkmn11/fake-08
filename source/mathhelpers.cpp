#include "mathhelpers.h"

//std::clamp is in c++17, but not c++11
int clamp (int val, int lo, int hi) {
	val = (val > hi) ? hi : val;
	return (val < lo) ? lo : val;
}

z8::fix32 lerp(z8::fix32 a, z8::fix32 b, z8::fix32 t) {
	return (b - a) * t + a;
}
