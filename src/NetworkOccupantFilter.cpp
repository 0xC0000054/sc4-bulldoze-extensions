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

#include "NetworkOccupantFilter.h"
#include "cISC4NetworkOccupant.h"
#include "cISC4Occupant.h"
#include "cRZAutoRefCount.h"

NetworkOccupantFilter::NetworkOccupantFilter(NetworkTypeFlags networkTypeFlags)
	: networkFlags(static_cast<uint32_t>(networkTypeFlags))
{
}

bool NetworkOccupantFilter::IsOccupantIncluded(cISC4Occupant* pOccupant)
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
