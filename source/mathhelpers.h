#pragma once

#include "fix32.h"

//std::clamp is in c++17, but not c++11
int clamp (int val, int lo, int hi);

z8::fix32 lerp(z8::fix32 a, z8::fix32 b, z8::fix32 t);
