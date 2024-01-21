#pragma once
#include <cstdint>
#include <gmp.h>
#include <vector>

namespace wild
{
	// https://github.com/garet90/MotorMC/blob/main/src/crypt/rsa.c
	class rsa_keypair
	{
		mpz_t d;
		mpz_t n;

	  public:
		std::vector<uint8_t> asn1;
		void generate();
	};
} // namespace wild
