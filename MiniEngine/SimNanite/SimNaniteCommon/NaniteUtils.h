#pragma once
#include <stdint.h>
#include <initializer_list>
#include <DirectXMath.h>

inline uint32_t MurmurFinalize32(uint32_t hash)
{
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash;
}

inline uint32_t Murmur32(std::initializer_list< uint32_t > init_list)
{
	uint32_t hash = 0;
	for (auto element : init_list)
	{
		element *= 0xcc9e2d51;
		element = (element << 15) | (element >> (32 - 15));
		element *= 0x1b873593;

		hash ^= element;
		hash = (hash << 13) | (hash >> (32 - 13));
		hash = hash * 5 + 0xe6546b64;
	}

	return MurmurFinalize32(hash);
}

inline uint32_t HashPosition(DirectX::XMFLOAT3 position)
{
	union { float f; uint32_t i; } x;
	union { float f; uint32_t i; } y;
	union { float f; uint32_t i; } z;

	x.f = position.x;
	y.f = position.y;
	z.f = position.z;

	return Murmur32({
		position.x == 0.0f ? 0u : x.i,
		position.y == 0.0f ? 0u : y.i,
		position.z == 0.0f ? 0u : z.i
		});
}

template <class T>
inline void HashCombine(uint64_t& s, const T& v)
{
	std::hash<T> h;
	s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}