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
#include <cstdint>

#ifdef __clang__
#define NAKED_FUN __attribute__((naked))
#else
#define NAKED_FUN __declspec(naked)
#endif

namespace Patcher
{
	void InstallJump(uintptr_t address, uintptr_t destination);

	void InstallJumpTableHook(uintptr_t targetAddress, uintptr_t newValue);

	void InstallCallHook(uintptr_t address, void (*pfnFunc)(void));
	void InstallCallHook(uintptr_t address, uintptr_t pfnFunc);

	void OverwriteMemory(uintptr_t address, uint8_t newValue);
}
