/*
 * This file is part of sc4-bulldoze-extensions, a DLL Plugin for
 * SimCity 4 extends the bulldoze tool.
 *
 * Copyright (C) 2024, 2025 Nicholas Hayes
 *
 * sc4-bulldoze-extensions is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * sc4-bulldoze-extensions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SC4ClearPollution.
 * If not, see <http://www.gnu.org/licenses/>.
 */

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

class NetworkOccupantFilterBase : public cSC4BaseOccupantFilter
{
protected:
	NetworkOccupantFilterBase(NetworkTypeFlags networkFlags);

	bool IsNetworkOccupant(cISC4Occupant* pOccupant) const;

private:
	uint32_t networkFlags;
};

