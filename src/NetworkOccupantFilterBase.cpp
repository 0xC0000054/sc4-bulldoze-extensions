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
 * along with sc4-bulldoze-extensions.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "NetworkOccupantFilterBase.h"
#include "cISC4NetworkOccupant.h"
#include "cISC4Occupant.h"
#include "cRZAutoRefCount.h"

NetworkOccupantFilterBase::NetworkOccupantFilterBase(NetworkTypeFlags networkTypeFlags)
	: networkFlags(static_cast<uint32_t>(networkTypeFlags))
{
}

bool NetworkOccupantFilterBase::IsNetworkOccupant(cISC4Occupant* pOccupant) const
{
	bool result = false;

	if (pOccupant)
	{
		cRZAutoRefCount<cISC4NetworkOccupant> networkOccupant;

		if (pOccupant->QueryInterface(GZIID_cISC4NetworkOccupant, networkOccupant.AsPPVoid()))
		{
			result = networkOccupant->HasAnyNetworkFlag(networkFlags);
		}
	}

	return result;
}
