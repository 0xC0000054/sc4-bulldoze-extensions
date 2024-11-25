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

#include "FloraOccupantFilter.h"
#include "cISC4Occupant.h"

FloraOccupantFilter::FloraOccupantFilter()
{
}

bool FloraOccupantFilter::IsOccupantTypeIncluded(uint32_t type)
{
	constexpr uint32_t kFloraOccupantType = 0x74758926;

	return type == kFloraOccupantType;
}
