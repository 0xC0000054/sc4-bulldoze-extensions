///////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-bulldoze-extensions, a DLL Plugin for SimCity 4
// that extends the bulldoze tool.
//
// Copyright (c) 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "cSC4BaseOccupantFilter.h"
#include <type_traits>

enum class NetworkTypeFlags : uint32_t
{
	Road = 1 << 0,
	Rail = 1 << 1,
	Highway = 1 << 2,
	Street = 1 << 3,
	WaterPipe = 1 << 4,
	PowerPole = 1 << 5,
	Avenue = 1 << 6,
	Subway = 1 << 7,
	LightRail = 1 << 8,
	Monorail = 1 << 9,
	OneWayRoad = 1 << 10,
	DirtRoad = 1 << 11,
	GroundHighway = 1 << 12,

	AllRoadNetworks = Road | Rail | Highway | Street | Avenue | OneWayRoad | DirtRoad | GroundHighway,
	AllRailNetworks = Rail | Subway | LightRail | Monorail,
	AllTransportationNetworks = AllRoadNetworks | AllRailNetworks
};

inline constexpr NetworkTypeFlags operator|(NetworkTypeFlags lhs, NetworkTypeFlags rhs)
{
	using T = std::underlying_type_t<NetworkTypeFlags>;

	return static_cast<NetworkTypeFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline NetworkTypeFlags& operator|=(NetworkTypeFlags& lhs, NetworkTypeFlags rhs)
{
	using T = std::underlying_type_t<NetworkTypeFlags>;

	return reinterpret_cast<NetworkTypeFlags&>(reinterpret_cast<T&>(lhs) |= static_cast<T>(rhs));
}

inline constexpr NetworkTypeFlags operator&(NetworkTypeFlags lhs, NetworkTypeFlags rhs)
{
	using T = std::underlying_type_t<NetworkTypeFlags>;

	return static_cast<NetworkTypeFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline NetworkTypeFlags& operator&=(NetworkTypeFlags& lhs, NetworkTypeFlags rhs)
{
	using T = std::underlying_type_t<NetworkTypeFlags>;

	return reinterpret_cast<NetworkTypeFlags&>(reinterpret_cast<T&>(lhs) &= static_cast<T>(rhs));
}

class NetworkOccupantFilter : public cSC4BaseOccupantFilter
{
public:
	NetworkOccupantFilter(NetworkTypeFlags networkFlags);

	bool IsOccupantIncluded(cISC4Occupant* pOccupant) override;

private:
	uint32_t networkFlags;
};

