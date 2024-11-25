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
#include <cstdint>

class SC4VersionDetection
{
public:

	static SC4VersionDetection& GetInstance();

	uint16_t GetGameVersion() const noexcept;

private:

	SC4VersionDetection();

	uint16_t gameVersion;
};

