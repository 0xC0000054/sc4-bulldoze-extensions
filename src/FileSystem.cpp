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

#include "FileSystem.h"
#include <Windows.h>
#include "wil/resource.h"
#include "wil/win32_helpers.h"

using namespace std::string_view_literals;

static constexpr std::string_view PluginConfigFileName = "SC4BulldozeExtensions.ini"sv;
static constexpr std::string_view PluginLogFileName = "SC4BulldozeExtensions.log"sv;

namespace
{
	std::filesystem::path GetDllFolderPathCore()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}

	std::filesystem::path GetDllFolderPath()
	{
		static std::filesystem::path path = GetDllFolderPathCore();

		return path;
	}
}

std::filesystem::path FileSystem::GetConfigFilePath()
{
	std::filesystem::path path = GetDllFolderPath();
	path /= PluginConfigFileName;

	return path;
}

std::filesystem::path FileSystem::GetLogFilePath()
{
	std::filesystem::path path = GetDllFolderPath();
	path /= PluginLogFileName;

	return path;
}
