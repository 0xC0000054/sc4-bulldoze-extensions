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
		 BulldozeCursorDefaultDiagonal = 0xE1855ADD,
		 BulldozeCursorFloraDiagonal = 0xED0E06AE,
		 BulldozeCursorNetworkDiagonal = 0x24ADE8F3,
	};

	cRZAutoRefCount<cISC4ViewInputControl> CreateViewInputControl(BulldozeCursor cursor);

	bool Install();
}