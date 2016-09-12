#include "IRremote.h"
#include "IRremoteInt.h"

//==============================================================================
//
//
//                              B A T H S C A L E
//
//
//==============================================================================

//
//  As per https://irq5.io/2014/09/25/cloud-enabling-a-bathroom-scale/
//
//  (result->value && 0xffff) is current weight in kgs,
//  result->value >> 16       is status byte
//
//  Tested on Sencor branded one.
//

#define BITS          39  // The number of bits in the command

#define HDR_MARK    1000  // The length of the Header:Mark
#define HDR_SPACE   2000  // The lenght of the Header:Space

#define BIT_MARK    500   // The length of a Bit:Mark
#define ONE_SPACE   500   // The length of a Bit:Space for 1's
#define ZERO_SPACE  1000  // The length of a Bit:Space for 0's

//+=============================================================================
//
#if SEND_BATHSCALE
void  IRsend::sendBathScale (uint64_t data)
{
	// Set IR carrier frequency
	enableIROut(38);

	// Data
	for (uint64_t  mask = 1UL << (BITS - 1);  mask;  mask >>= 1) {
		if (data & mask) {
			mark (BIT_MARK);
			space(ONE_SPACE);
		} else {
			mark (BIT_MARK);
			space(ZERO_SPACE);
		}
	}

	// Footer
	mark(BIT_MARK);
	space(0);  // Always end with the LED off
}
#endif

//+=============================================================================
//
#if DECODE_BATHSCALE
bool  IRrecv::decodeBathScale (decode_results *results)
{
	uint64_t  data   = 0;  // Somewhere to build our code
	int       offset = 1;  // Skip the Gap reading

	// Check we have the right amount of data
	if (irparams.rawlen != 1 + (2 * BITS) + 1)  return false ;

	// Read the bits in
	for (int i = 0;  i < BITS;  i++) {
		// Each bit looks like: MARK + SPACE_1 -> 1
		//                 or : MARK + SPACE_0 -> 0
		if (!MATCH_MARK(results->rawbuf[offset++], BIT_MARK))  return false ;

		// IR data is big-endian, so we shuffle it in from the right:
		if      (MATCH_SPACE(results->rawbuf[offset], ONE_SPACE))   data = (data << 1) | 1 ;
		else if (MATCH_SPACE(results->rawbuf[offset], ZERO_SPACE))  data = (data << 1) | 0 ;
		else                                                        return false ;
		offset++;
	}

	uint8_t checksum = (data & 0x7f);
	data >>= 7;

	if(data >> 24 != 0xab)
		return false; // No preamble

	uint16_t calc_checksum = 0;
	for(uint32_t d = data; d; d >>= 8) {
		calc_checksum += d & 0xff;
		calc_checksum %= 0xff;
	}
	calc_checksum >>= 1;

	if(calc_checksum != checksum)
		return false; // Invalid checksum

	// Success
	results->bits        = 24;
	results->value       = data & 0xffffff;
	results->decode_type = BATHSCALE;
	return true;
}
#endif
