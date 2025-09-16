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

#include "DezoneKeepNetworksOccupantFilter.h"
#include "cISC4Lot.h"
#include "cISC4LotManager.h"
#include "cISC4Occupant.h"
#include "cRZAutoRefCount.h"
#include "GlobalCityPointers.h"

using ZoneType = cISC4ZoneManager::ZoneType;

DezoneKeepNetworksOccupantFilter::DezoneKeepNetworksOccupantFilter()
	: NetworkOccupantFilterBase(NetworkTypeFlags::AllTransportationNetworks)
{
}

bool DezoneKeepNetworksOccupantFilter::IsOccupantIncluded(cISC4Occupant* pOccupant)
{
	bool result = false;

	// Exclude all networks from demolition, only the zoned areas will be demolished.

	if (!IsNetworkOccupant(pOccupant) && spLotManager)
	{
		cISC4Lot* pLot = spLotManager->GetOccupantLot(pOccupant);

		if (pLot)
		{
			// We limit the tool to RCI zones.
			// The Maxis dezone tool ignores the Plopped zone type, but unlike our version
			// it can remove empty landfill zones.
			// Landfill zones being ignored may be an artifact of the dezoning implementation
			// in cISC4Demolition::DemolishRegion.

			const ZoneType zoneType = pLot->GetZoneType();

			result = zoneType >= ZoneType::ResidentialLowDensity && zoneType <= ZoneType::IndustrialHighDensity;
		}
	}

	return result;
}
