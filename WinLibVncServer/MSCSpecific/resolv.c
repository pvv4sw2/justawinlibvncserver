#include <stdlib.h>
#include <ctype.h>

#include "resolv.h"

static const char Base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

#define Assert(Cond)

int __b64_ntop(uint8_t const *src, size_t srclength, char *target, size_t targsize) {
	size_t datalength = 0;
	uint8_t input[3];
	uint8_t output[4];
	size_t i;

	while (2 < srclength) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclength -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;
		Assert(output[0] < 64);
		Assert(output[1] < 64);
		Assert(output[2] < 64);
		Assert(output[3] < 64);

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = Base64[output[0]];
		target[datalength++] = Base64[output[1]];
		target[datalength++] = Base64[output[2]];
		target[datalength++] = Base64[output[3]];
	}

	/* Now we worry about padding. */
	if (0 != srclength) {
		/* Get what's left. */
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclength; i++)
			input[i] = *src++;
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		Assert(output[0] < 64);
		Assert(output[1] < 64);
		Assert(output[2] < 64);

		if (datalength + 4 > targsize)
			return (-1);
		target[datalength++] = Base64[output[0]];
		target[datalength++] = Base64[output[1]];
		if (srclength == 1)
			target[datalength++] = Pad64;
		else
			target[datalength++] = Base64[output[2]];
		target[datalength++] = Pad64;
	}
	if (datalength >= targsize)
		return (-1);
	target[datalength] = '\0'; /* Returned value doesn't count \0. */
	return (datalength);
}

static int b64rmap_initialized = 0;
static uint8_t b64rmap[256];

static const uint8_t b64rmap_special = 0xf0;
static const uint8_t b64rmap_end = 0xfd;
static const uint8_t b64rmap_space = 0xfe;
static const uint8_t b64rmap_invalid = 0xff;

/**
 * Initializing the reverse map is not thread safe.
 * Which is fine for NSD. For now...
 **/
static void
b64_initialize_rmap ()
{
	int i;
	char ch;

	/* Null: end of string, stop parsing */
	b64rmap[0] = b64rmap_end;

	for (i = 1; i < 256; ++i) {
		ch = (char)i;
		/* Whitespaces */
        if (isspace(i))
			b64rmap[i] = b64rmap_space;
		/* Padding: stop parsing */
		else if (ch == Pad64)
			b64rmap[i] = b64rmap_end;
		/* Non-base64 char */
		else
			b64rmap[i] = b64rmap_invalid;
	}

	/* Fill reverse mapping for base64 chars */
	for (i = 0; Base64[i] != '\0'; ++i)
		b64rmap[(uint8_t)Base64[i]] = i;

	b64rmap_initialized = 1;
}

static int
b64_pton_do(char const *src, uint8_t *target, size_t targsize)
{
	int tarindex, state, ch;
	uint8_t ofs;

	state = 0;
	tarindex = 0;

	while (1)
	{
		ch = *src++;
		ofs = b64rmap[ch];

		if (ofs >= b64rmap_special) {
			/* Ignore whitespaces */
			if (ofs == b64rmap_space)
				continue;
			/* End of base64 characters */
			if (ofs == b64rmap_end)
				break;
			/* A non-base64 character. */
			return (-1);
		}

		switch (state) {
		case 0:
			if ((size_t)tarindex >= targsize)
				return (-1);
			target[tarindex] = ofs << 2;
			state = 1;
			break;
		case 1:
			if ((size_t)tarindex + 1 >= targsize)
				return (-1);
			target[tarindex]   |=  ofs >> 4;
			target[tarindex+1]  = (ofs & 0x0f)
						<< 4 ;
			tarindex++;
			state = 2;
			break;
		case 2:
			if ((size_t)tarindex + 1 >= targsize)
				return (-1);
			target[tarindex]   |=  ofs >> 2;
			target[tarindex+1]  = (ofs & 0x03)
						<< 6;
			tarindex++;
			state = 3;
			break;
		case 3:
			if ((size_t)tarindex >= targsize)
				return (-1);
			target[tarindex] |= ofs;
			tarindex++;
			state = 0;
			break;
		default:
			abort();
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == Pad64) {		/* We got a pad char. */
		ch = *src++;		/* Skip it, get next. */
		switch (state) {
		case 0:		/* Invalid = in first position */
		case 1:		/* Invalid = in second position */
			return (-1);

		case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (b64rmap[ch] != b64rmap_space)
					break;
			/* Make sure there is another trailing = sign. */
			if (ch != Pad64)
				return (-1);
			ch = *src++;		/* Skip the = */
			/* Fall through to "single trailing =" case. */
			/* FALLTHROUGH */

		case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (b64rmap[ch] != b64rmap_space)
					return (-1);

			/*
			 * Now make sure for cases 2 and 3 that the "extra"
			 * bits that slopped past the last full byte were
			 * zeros.  If we don't check them, they become a
			 * subliminal channel.
			 */
			if (target[tarindex] != 0)
				return (-1);
		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}

	return (tarindex);
}


static int
b64_pton_len(char const *src)
{
	int tarindex, state, ch;
	uint8_t ofs;

	state = 0;
	tarindex = 0;

	while (1)
	{
		ch = *src++;
		ofs = b64rmap[ch];

		if (ofs >= b64rmap_special) {
			/* Ignore whitespaces */
			if (ofs == b64rmap_space)
				continue;
			/* End of base64 characters */
			if (ofs == b64rmap_end)
				break;
			/* A non-base64 character. */
			return (-1);
		}

		switch (state) {
		case 0:
			state = 1;
			break;
		case 1:
			tarindex++;
			state = 2;
			break;
		case 2:
			tarindex++;
			state = 3;
			break;
		case 3:
			tarindex++;
			state = 0;
			break;
		default:
			abort();
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == Pad64) {		/* We got a pad char. */
		ch = *src++;		/* Skip it, get next. */
		switch (state) {
		case 0:		/* Invalid = in first position */
		case 1:		/* Invalid = in second position */
			return (-1);

		case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (b64rmap[ch] != b64rmap_space)
					break;
			/* Make sure there is another trailing = sign. */
			if (ch != Pad64)
				return (-1);
			ch = *src++;		/* Skip the = */
			/* Fall through to "single trailing =" case. */
			/* FALLTHROUGH */

		case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (b64rmap[ch] != b64rmap_space)
					return (-1);

		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}

	return (tarindex);
}


int __b64_pton(char const *src, uint8_t *target, size_t targsize)
{
	if (!b64rmap_initialized)
		b64_initialize_rmap ();

	if (target)
		return b64_pton_do (src, target, targsize);
	else
		return b64_pton_len (src);
}
