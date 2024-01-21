#include "net/crypto/rsa_keypair.hpp"
#include <algorithm>
#include <random>

namespace wild
{
	std::random_device rd;
	std::default_random_engine engine(rd());

	void generate_prime(mpz_t prime, size_t size)
	{
		uint8_t data[size];

		std::generate(data, data + size, std::ref(engine));

		data[size - 1] |= 0x1;
		data[0] |= 0xC0;

		mpz_import(prime, size, 1, sizeof(uint8_t), 0, 0, data);
		mpz_nextprime(prime, prime);
	}

	// https://github.com/garet90/MotorMC/blob/main/src/crypt/rsa.c
	// i honestly have no idea about anything crypto related.
	void rsa_keypair::generate()
	{
		mpz_t e;
		mpz_init_set_ui(e, 65537);

		mpz_t p, q;
		mpz_init2(p, 512);
		mpz_init2(q, 512);

		generate_prime(p, 64);
		generate_prime(q, 64);

		mpz_init2(this->n, 1024);
		mpz_mul(this->n, p, q);

		mpz_init2(this->d, 1024);

		mpz_sub_ui(p, p, 1);
		mpz_sub_ui(q, q, 1);

		mpz_lcm(p, q, p);

		mpz_invert(this->d, e, p);

		mpz_clears(e, p, q, NULL);

		const uint8_t prefix[] = {0x30, 0x81, 0x9F, 0x30, 0x0D, 0x06, 0x09,
								  0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01,
								  0x01, 0x01, 0x05, 0x00, 0x03, 0x81, 0x8D,
								  0x00, 0x30, 0x81, 0x89, 0x02, 0x81, 0x81};

		const uint8_t suffix[] = {0x02, 0x03, 0x01, 0x00, 0x01};

		uint8_t data_buffer[256];

		memcpy(data_buffer, prefix, sizeof(prefix));
		mpz_export(data_buffer + sizeof(prefix) + 1, NULL, 1, sizeof(uint8_t),
				   0, 0, this->n);
		memcpy(data_buffer + sizeof(prefix) + 129, suffix, sizeof(suffix));

		this->asn1 = std::vector<uint8_t>(data_buffer, data_buffer + 256);
	}
} // namespace wild
