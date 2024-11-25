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

class FloraOccupantFilter : public cSC4BaseOccupantFilter
{
public:
	FloraOccupantFilter();

	bool IsOccupantTypeIncluded(uint32_t type) override;
};

