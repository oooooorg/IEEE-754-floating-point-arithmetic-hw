#include "return_codes.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct singlePrecision
{
	bool sign;
	int8_t exponent;
	uint64_t mantissa;
	uint8_t lost;
	uint8_t lastLost;
	bool expOver;
} singlePrecision;

void printInf(bool sign)
{
	if (sign)
	{
		printf("-inf\n");
	}
	else
	{
		printf("inf\n");
	}
}

uint8_t len_int(uint64_t number)
{
	uint8_t len = 0;
	while (number != 0)
	{
		number >>= 4;
		len++;
	}
	return len;
}

bool checkNan(singlePrecision struc, uint8_t mantissaLen, uint8_t expLen)
{
	return struc.exponent == -(1 << expLen - 1) && struc.mantissa != 1 << mantissaLen;
}

uint8_t checkInf(singlePrecision struc, uint8_t mantissaLen, uint8_t expLen)
{
	if (struc.exponent == -(1 << expLen - 1) || struc.exponent == 1 && struc.mantissa == (1 << mantissaLen))
	{
		if (struc.sign)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
	return 0;
}

bool checkZero(singlePrecision struc, uint8_t mantissaLen, uint8_t expLen)
{
	if (struc.mantissa == 1 << mantissaLen && struc.exponent == -1 * (1 << expLen - 1) + 1)
	{
		return 1;
	}
	return 0;
}

void print(singlePrecision struc, uint8_t size, uint8_t rounding)
{
	uint8_t sdvig;
	uint8_t mantissaLen;
	uint8_t expLen;
	if (size == 6)
	{
		sdvig = 1;
		mantissaLen = 23;
		expLen = 8;
	}
	else
	{
		sdvig = 2;
		mantissaLen = 10;
		expLen = 5;
	}

	bool symb = 1;

	if (checkZero(struc, mantissaLen, expLen))
	{
		struc.exponent = 0;
		symb = 0;
	}
	if (checkNan(struc, mantissaLen, expLen))
	{
		printf("nan\n");
		return;
	}
	if (checkInf(struc, mantissaLen, expLen))
	{
		printInf(struc.sign);
		return;
	}
	struc.mantissa -= (1 << mantissaLen);
	if (((rounding == 1 && struc.lastLost == 1 && struc.lost > 1) || rounding == 1 && struc.lastLost == 1 && struc.mantissa % 2 != 0) ||
		rounding == 2 && struc.lost >= 1 && !struc.sign)
	{
		struc.mantissa++;
	}
	if (rounding == 3 && struc.lost >= 1 && struc.sign)
	{
		struc.mantissa--;
	}
	struc.mantissa <<= sdvig;
	if (struc.sign)
	{
		printf("-");
	}
	if (struc.expOver)
	{
		printf("0x%d.%0*llxp%+d\n", symb, size, struc.mantissa, (int16_t)(struc.exponent - 256));
	}
	else
	{
		printf("0x%d.%0*llxp%+d\n", symb, size, struc.mantissa, struc.exponent);
	}
}

void out(singlePrecision struc, uint8_t mantissaLen, uint8_t rounding)
{
	switch (mantissaLen)
	{
	case 23:
		print(struc, 6, rounding);
		break;
	case 10:
		print(struc, 3, rounding);
		break;
	default:;
	}
}

singlePrecision normalize(singlePrecision struc, uint8_t mantissaLen)
{
	if (struc.mantissa > (1 << mantissaLen + 1))
	{
		while (struc.mantissa > 1 << mantissaLen + 1)
		{
			uint64_t temp = struc.mantissa;
			struc.mantissa >>= 1;
			if ((struc.mantissa << 1) + 1 == temp)
			{
				struc.lost++;
				struc.lastLost = 1;
			}
			else
			{
				struc.lastLost = 0;
			}
		}
		struc.exponent += 1;
	}
	return struc;
}

singlePrecision convert(int32_t intValue, uint8_t mantissaLen, uint8_t expLen)
{
	uint32_t norMan = mantissaLen == 23 ? 0x7FFFFF : 0x3FF;
	uint16_t norLen = expLen == 8 ? 0xFF : 0x1F;
	singlePrecision num = {
		intValue >> (mantissaLen + expLen) & 1,
		(intValue >> mantissaLen & norLen) - (1 << expLen - 1) + 1,
		(intValue & norMan),
		0,
		0,
		false,
	};
	uint8_t exp = expLen == 8 ? (1 << 7) - 1 : (1 << 4) - 1;
	int16_t expOver = 0;
	if (num.exponent == -exp & num.mantissa != 0)
	{
		uint8_t sdvig = mantissaLen - len_int(num.mantissa);
		num.mantissa <<= sdvig;
		expOver = 1 - exp - sdvig;
	}
	else
	{
		num.mantissa += (1 << mantissaLen);
	}
	if (expOver != 0)
	{
		num.exponent = expOver;
		num.expOver = 1;
	}
	return num;
}

void multiply(singlePrecision num1, singlePrecision num2, uint8_t mantissaLen, uint8_t rounding)
{
	uint8_t expLen = mantissaLen == 23 ? 8 : 5;
	num1.sign ^= num2.sign;
	if ((checkZero(num1, mantissaLen, expLen) && checkInf(num2, mantissaLen, expLen)) ||
		(checkZero(num2, mantissaLen, expLen) && checkInf(num1, mantissaLen, expLen)) ||
		(checkNan(num1, mantissaLen, expLen) || checkNan(num2, mantissaLen, expLen)))
	{
		printf("nan\n");
		return;
	}
	if (checkInf(num1, mantissaLen, expLen) || checkInf(num2, mantissaLen, expLen))
	{
		printInf(num1.sign);
		return;
	}
	if (checkZero(num1, mantissaLen, expLen))
	{
		out(normalize(num1, mantissaLen), mantissaLen, rounding);
		return;
	}
	else if (checkZero(num2, mantissaLen, expLen))
	{
		num2.sign = num1.sign;
		out(normalize(num2, mantissaLen), mantissaLen, rounding);
		return;
	}
	num1.exponent += num2.exponent;
	num1.mantissa *= num2.mantissa;
	num1 = normalize(num1, mantissaLen);
	out(num1, mantissaLen, rounding);
}

void printSign(singlePrecision num, uint8_t mantissaLen, uint8_t expLen)
{
	if (checkInf(num, mantissaLen, expLen))
	{
		printInf(num.sign);
		return;
	}
}

void divide(singlePrecision num1, singlePrecision num2, uint8_t mantissaLen, uint8_t rounding, int32_t b)
{
	uint8_t expLen = mantissaLen == 23 ? 8 : 5;
	num1.sign ^= num2.sign;
	if (checkZero(num1, mantissaLen, expLen) && checkZero(num2, mantissaLen, expLen))
	{
		printf("nan\n");
		return;
	}
	if (checkZero(num1, mantissaLen, expLen))
	{
		out(normalize(num1, mantissaLen), mantissaLen, rounding);
		return;
	}
	if ((checkInf(num1, mantissaLen, expLen) && checkInf(num2, mantissaLen, expLen)) ||
		(checkNan(num1, mantissaLen, expLen) || checkNan(num2, mantissaLen, expLen)))
	{
		printf("nan\n");
		return;
	}
	if (checkInf(num1, mantissaLen, expLen) || checkZero(num2, mantissaLen, expLen))
	{
		printInf(num1.sign);
		return;
	}
	if (checkZero(num2, mantissaLen, expLen))
	{
		if (b > 0)
		{
			printInf(0);
			return;
		}
		else
		{
			printInf(1);
			return;
		}
	}
	num1.mantissa <<= (64 - mantissaLen - 1);
	num1.exponent -= num2.exponent + 1;
	num1.mantissa /= num2.mantissa;
	num1.exponent -= (num1.mantissa >> (64 - mantissaLen - 1) < 1);

	out(normalize(num1, mantissaLen), mantissaLen, rounding);
}

singlePrecision checkMant(singlePrecision num1, singlePrecision num2)
{
	if (num1.mantissa < num2.mantissa)
	{
		num1.sign = 1;
		num1.mantissa = num2.mantissa - num1.mantissa;
	}
	else
	{
		num1.mantissa -= num2.mantissa;
	}
	return num1;
}

void minus(singlePrecision num1, singlePrecision num2, uint8_t mantissaLen, uint8_t expLen, uint8_t rounding)
{
	if (checkNan(num1, mantissaLen, expLen) || checkNan(num2, mantissaLen, expLen))
	{
		printf("nan\n");
		return;
	}
	if (checkInf(num1, mantissaLen, expLen))
	{
		printInf(num1.sign);
		return;
	}
	uint8_t delta = abs(num1.exponent - num2.exponent);
	if (num1.exponent > num2.exponent)
	{
		num1.mantissa <<= delta;
		num1.exponent = num2.exponent;
		num1 = checkMant(num1, num2);
	}
	else
	{
		num1.mantissa >>= delta;
		num1.exponent = num2.exponent;
		num1 = checkMant(num1, num2);
	}

	out(normalize(num1, mantissaLen), mantissaLen, rounding);
}

void minusPrep(int32_t intValue1, int32_t intValue2, uint8_t mantissaLen, uint8_t expLen, uint8_t rounding)
{
	singlePrecision num1 = convert(intValue1, mantissaLen, expLen);
	singlePrecision num2 = convert(intValue2, mantissaLen, expLen);
	num1.sign = intValue1 < intValue2 ? 1 : 0;
	minus(num1, num2, mantissaLen, expLen, rounding);
}

void plus(singlePrecision num1, singlePrecision num2, uint8_t mantissaLen, uint8_t rounding)
{
	uint8_t expLen = mantissaLen == 23 ? 8 : 5;
	if (checkZero(num1, mantissaLen, expLen))
	{
		out(normalize(num2, mantissaLen), mantissaLen, rounding);
		return;
	}
	else if (checkZero(num2, mantissaLen, expLen))
	{
		out(normalize(num1, mantissaLen), mantissaLen, rounding);
		return;
	}
	if ((checkInf(num1, mantissaLen, expLen) == 2 && checkInf(num2, mantissaLen, expLen) == 1) ||
		(checkInf(num1, mantissaLen, expLen) == 1 && checkInf(num2, mantissaLen, expLen) == 2) ||
		(checkNan(num1, mantissaLen, expLen) || checkNan(num2, mantissaLen, expLen)))
	{
		printf("nan\n");
		return;
	}
	if (checkZero(num1, mantissaLen, expLen) && checkZero(num2, mantissaLen, expLen))
	{
		num1.sign = 0;
		out(num1, mantissaLen, rounding);
		return;
	}
	printSign(num1, mantissaLen, expLen);
	printSign(num2, mantissaLen, expLen);

	if (!num1.sign && !num2.sign)
	{
		uint8_t delta = abs(num1.exponent - num2.exponent);
		if (num1.exponent > num2.exponent)
		{
			num2.mantissa >>= delta;
			num1.mantissa += num2.mantissa;
		}
		else
		{
			num1.mantissa >>= delta;
			num1.exponent = num2.exponent;
			num1.mantissa += num2.mantissa;
		}

		out(normalize(num1, mantissaLen), mantissaLen, rounding);
	}
}

int main(int argc, char* argv[])
{
	int32_t b;
	if (argc != 4 && argc != 6)
	{
		fprintf(stderr, "no arguments");
		return ERROR_ARGUMENTS_INVALID;
	}
	uint8_t rounding = 0;
	if (!sscanf(argv[2], "%hhd", &rounding))
	{
		fprintf(stderr, "no rounding");
		return ERROR_ARGUMENTS_INVALID;
	};
	if (!sscanf(argv[3], "%x", &b))
	{
		fprintf(stderr, "no value");
		return ERROR_ARGUMENTS_INVALID;
	};
	if (strcmp(argv[1], "f") != 0 && strcmp(argv[1], "h") != 0)
	{
		fprintf(stderr, "incorrect format");
		return ERROR_ARGUMENTS_INVALID;
	}
	if (rounding >= 4 || rounding < 0)
	{
		fprintf(stderr, "incorrect rounding");
		return ERROR_ARGUMENTS_INVALID;
	}

	uint8_t mantissaLen;
	uint8_t expLen;
	if (strcmp(argv[1], "f") == 0)
	{
		mantissaLen = 23;
		expLen = 8;
	}
	else
	{
		mantissaLen = 10;
		expLen = 5;
	}
	if (argc == 4)
	{
		out(convert(b, mantissaLen, expLen), mantissaLen, rounding);
	}
	else
	{
		int32_t c;
		if (!sscanf(argv[5], "%x", &c))
		{
			fprintf(stderr, "no value");
			return ERROR_ARGUMENTS_INVALID;
		}
		singlePrecision bs = convert(b, mantissaLen, expLen);
		singlePrecision cs = convert(c, mantissaLen, expLen);

		if (strcmp(argv[4], "*") == 0)
		{
			multiply(bs, cs, mantissaLen, rounding);
		}
		else if (strcmp(argv[4], "/") == 0)
		{
			divide(bs, cs, mantissaLen, rounding, b);
		}
		else if (strcmp(argv[4], "+") == 0)
		{
			plus(bs, cs, mantissaLen, rounding);
		}
		else if (strcmp(argv[4], "-") == 0)
		{
			minusPrep(b, c, mantissaLen, expLen, rounding);
		}
		else
		{
			fprintf(stderr, "incorrect operation");
			return ERROR_ARGUMENTS_INVALID;
		}
	}
	return SUCCESS;
}
