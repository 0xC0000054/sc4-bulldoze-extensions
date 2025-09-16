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

#pragma once
#include "NetworkOccupantFilterBase.h"

class KeepNetworksOccupantFilter :  public NetworkOccupantFilterBase
{
public:
	KeepNetworksOccupantFilter(NetworkTypeFlags networkFlags);

	bool IsOccupantIncluded(cISC4Occupant* pOccupant) override;
};

