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
