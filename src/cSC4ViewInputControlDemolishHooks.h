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
#include "cISC4ViewInputControl.h"
#include "cRZAutoRefCount.h"
#include <cstdint>

namespace cSC4ViewInputControlDemolishHooks
{
	enum BulldozeCursor : uint32_t
	{
		 BulldozeCursorDefault = 0xE1855ADC,
		 BulldozeCursorFlora = 0xED0E06AD,
		 BulldozeCursorNetwork = 0x24ADE8F2,
	};

	cRZAutoRefCount<cISC4ViewInputControl> CreateViewInputControl(BulldozeCursor cursor);

	bool Install();
}