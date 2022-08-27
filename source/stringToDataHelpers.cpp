#include <string>

#include "stringToDataHelpers.h"

#include "logger.h"

int char_to_int(char c) {
	// This is a bit faster than an if and
	// *much* faster than strtol
	switch(c) {
		default:
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a':
		case 'A': return 0xA;
		case 'b':
		case 'B': return 0xB;
		case 'c':
		case 'C': return 0xC;
		case 'd':
		case 'D': return 0xD;
		case 'e':
		case 'E': return 0xE;
		case 'f':
		case 'F': return 0xF;
	}
}

void copy_string_to_sprite_memory(uint8_t sprite_data[128 * 64], std::string data) {
	uint16_t i = 0;

	for (size_t n = 0; n < data.length(); n++) {
		//https://pico-8.fandom.com/wiki/Memory
		// "An 8-bit byte represents two pixels, horizontally adjacent, where the most significant 
		// (leftmost) 4 bits is the right pixel of the pair, and the least significant 4 bits is 
		// the left pixel.
		if (data[n] > ' ') {
			uint8_t val = char_to_int(data[n++]); //left pixel
			val |= char_to_int(data[n]) << 4;     //right pixel
			sprite_data[i++] = val;
		}
	}
}

void copy_mini_label_to_sprite_memory(uint8_t sprite_data[128 * 64], std::string data, int labeloffset) {
	int buffcount = 0;
	int labelx = 0;
	int labely = 0;
	uint8_t val;
	
	for (size_t n = 0; n < data.length(); n++) {
		
		if (data[n] > ' ') {
			
			if (buffcount == 0) {
				val = char_to_int(data[n]);
			}
			
			if (buffcount == 4) {
				val |= char_to_int(data[n]) << 4;
				if ((labelx + (labely * 16)) % 64 <= 15){
					sprite_data[labeloffset + labelx + (labely * 16)] = val;
				}
				labelx++;
				
			}
			
			if(labelx == 16){
				labely++;
				labelx = 0;
			}
			
			buffcount++;
			
			if (buffcount == 8){
				buffcount = 0;
			}
			
		}
	}
}


void copy_string_to_memory(uint8_t* sprite_flag_data, std::string data) {
	uint16_t i = 0;

	for (size_t n = 0; n < data.length(); n++) {
		if (data[n] > ' ') {
			uint8_t val = char_to_int(data[n++]); //left pixel
			val |= char_to_int(data[n]) << 4;     //right pixel
			sprite_flag_data[i++] = val;
		}
	}
}