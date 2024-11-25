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

#include "Patcher.h"
#include <Windows.h>
#include "wil/resource.h"
#include "wil/win32_helpers.h"

void Patcher::InstallJump(uintptr_t address, uintptr_t destination)
{
	DWORD oldProtect;
	THROW_IF_WIN32_BOOL_FALSE(VirtualProtect(reinterpret_cast<void*>(address), 5, PAGE_EXECUTE_READWRITE, &oldProtect));

	*((uint8_t*)address) = 0xE9;
	*((uintptr_t*)(address + 1)) = destination - address - 5;
}

void Patcher::InstallJumpTableHook(uintptr_t targetAddress, uintptr_t newValue)
{
	// Allow the executable memory to be written to.
	DWORD oldProtect = 0;
	THROW_IF_WIN32_BOOL_FALSE(VirtualProtect(
		reinterpret_cast<LPVOID>(targetAddress),
		sizeof(uintptr_t),
		PAGE_EXECUTE_READWRITE,
		&oldProtect));

	// Patch the memory at the specified address.
	*reinterpret_cast<uintptr_t*>(targetAddress) = newValue;
}

void Patcher::InstallCallHook(uintptr_t address, void(*pfnFunc)(void))
{
	InstallCallHook(address, reinterpret_cast<uintptr_t>(pfnFunc));
}

void Patcher::InstallCallHook(uintptr_t address, uintptr_t pfnFunc)
{
	DWORD oldProtect;
	THROW_IF_WIN32_BOOL_FALSE(VirtualProtect(reinterpret_cast<void*>(address), 5, PAGE_EXECUTE_READWRITE, &oldProtect));

	*((uint8_t*)address) = 0xE8;
	*((uintptr_t*)(address + 1)) = pfnFunc - address - 5;
}

void Patcher::OverwriteMemory(uintptr_t address, uint8_t newValue)
{
	DWORD oldProtect;
	// Allow the executable memory to be written to.
	THROW_IF_WIN32_BOOL_FALSE(VirtualProtect(
		reinterpret_cast<void*>(address),
		sizeof(newValue),
		PAGE_EXECUTE_READWRITE,
		&oldProtect));

	// Patch the memory at the specified address.
	*((uint8_t*)address) = newValue;
}
