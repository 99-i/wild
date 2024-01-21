#pragma once
#include <cmath>
#include <string>

namespace wild
{
struct vec3i
{
	int x;
	int y;
	int z;
	vec3i(int x, int y, int z) : x(x), y(y), z(z) {}
	bool operator==(const vec3i &other)
	{
		return (other.x == this->x && other.y == this->y && other.z == this->z);
	}
};
struct vec3d
{
	double x;
	double y;
	double z;
	vec3d() : x(0), y(0), z(0) {}
	vec3d(double x, double y, double z) : x(x), y(y), z(z) {}
	double distance(const vec3d &other)
	{
		return sqrt(pow(other.x - this->x, 2) + pow(other.y - this->y, 2) +
					pow(other.z - this->z, 2));
	}
	double distance_squared(const vec3d &other)
	{
		return (pow(other.x - this->x, 2) + pow(other.y - this->y, 2) +
				pow(other.z - this->z, 2));
	}
	vec3d operator+(const vec3d &other) const
	{
		return vec3d(this->x + other.x, this->y + other.y, this->z + other.z);
	}
	bool operator==(const vec3d &other)
	{
		return (other.x == this->x && other.y == this->y && other.z == this->z);
	}
};

enum class client_state
{
	HANDSHAKING,
	STATUS,
	LOGIN,
	PLAY
};

} // namespace wild
