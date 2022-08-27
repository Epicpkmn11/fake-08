
#include <string>

#include "hostVmShared.h"
#include "nibblehelpers.h"



int getCombinedIdx(const int x, const int y){
	//bit shifting might be faster? trying to optimize
    //return y * 64 + (x / 2);
	//return (y << 6) | (x >> 1);
	return COMBINED_IDX(x, y);
}
//try look up table to optimize?
