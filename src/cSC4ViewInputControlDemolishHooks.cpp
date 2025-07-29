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

#include "cSC4ViewInputControlDemolishHooks.h"
#include "cIGZAllocatorService.h"
#include "cISC4Demolition.h"
#include "cISC4OccupantFilter.h"
#include "cRZAutoRefCount.h"
#include "FloraOccupantFilter.h"
#include "GZServPtrs.h"
#include "Logger.h"
#include "NetworkOccupantFilter.h"
#include "Patcher.h"
#include "SC4CellRegion.h"
#include "SC4List.h"
#include "SC4VersionDetection.h"
#include <Windows.h>
#include "wil/result.h"
#include <cstdint>

namespace
{
	struct S3DColorFloat
	{
		float r;
		float g;
		float b;
		float a;
	};

	struct cSC4ViewInputControlDemolish : public cISC4ViewInputControl
	{
		uint8_t bInitialized;
		uint32_t refCount;
		uint32_t id;
		uint32_t cursorIID;
		void* pCursor;										// cIGZCursor*
		void* pWindow;										// cIGZWin*
		void* pView3DWin;									// cISC4View3DWin *
		void* pWM;											// cIGZWinMgr*
		intptr_t unknown1;
		void* pBudgetSim;									// cISC4BudgetSimulator*
		void* pCity;										// cISC4City*
		cISC4Demolition* pDemolition;
		void* pLotDeveloper;								// cISC4LotDeveloper*
		void* pLotManager;									// cISC4LotManager*
		cISC4OccupantFilter* pDemolishableOccupantFilter;
		void* pOccupantManager;								// cISC4OccupantManager*
		uint8_t bCellPicked;
		uint8_t unknown2[27];
		int32_t cellPointX;
		int32_t cellPointZ;
		SC4CellRegion<int32_t>* pCellRegion;
		uint8_t bValidDemolitionTarget;
		void* pSelectedOccupant;							// cISC4Occupant*
		uint8_t unknown3[28];
		void* pMarkedCellView;
		uint8_t bSignPostOccupant;
		S3DColorFloat destroyOK;
		S3DColorFloat destroyNotOK;
		S3DColorFloat demolishOK;
		S3DColorFloat demolishNotOK;
	};

	static_assert(sizeof(cSC4ViewInputControlDemolish) == 0xd8);
	static_assert(offsetof(cSC4ViewInputControlDemolish, id) == 0xc);
	static_assert(offsetof(cSC4ViewInputControlDemolish, pBudgetSim) == 0x28);
	static_assert(offsetof(cSC4ViewInputControlDemolish, pOccupantManager) == 0x40);
	static_assert(offsetof(cSC4ViewInputControlDemolish, bCellPicked) == 0x44);
	static_assert(offsetof(cSC4ViewInputControlDemolish, cellPointX) == 0x60);
	static_assert(offsetof(cSC4ViewInputControlDemolish, pCellRegion) == 0x68);
	static_assert(offsetof(cSC4ViewInputControlDemolish, pMarkedCellView) == 0x90);

	typedef cSC4ViewInputControlDemolish* (__thiscall* PFN_cSC4ViewInputControlDemolish_ctor)(cSC4ViewInputControlDemolish* pThis);
	static const PFN_cSC4ViewInputControlDemolish_ctor cSC4ViewInputControlDemolish_ctor = reinterpret_cast<PFN_cSC4ViewInputControlDemolish_ctor>(0x4b9070);

	enum class OccupantFilterType
	{
		None = 0,
		Flora = 1,
		Network = 2
	};

	static OccupantFilterType occupantFilterType = OccupantFilterType::None;

	typedef bool(__thiscall* cSC4ViewInputControl_IsOnTop)(cISC4ViewInputControl* pThis);

	static const cSC4ViewInputControl_IsOnTop IsOnTop = reinterpret_cast<cSC4ViewInputControl_IsOnTop>(0x5fb190);

	typedef void(__thiscall* cSC4ViewInputControlDemolish_ThiscallFn)(cSC4ViewInputControlDemolish* pThis);

	static const cSC4ViewInputControlDemolish_ThiscallFn EndInput = reinterpret_cast<cSC4ViewInputControlDemolish_ThiscallFn>(0x4b9040);
	static const cSC4ViewInputControlDemolish_ThiscallFn UpdateSelectedRegion = reinterpret_cast<cSC4ViewInputControlDemolish_ThiscallFn>(0x4b93b0);

	void SetOccupantFilterOption(cSC4ViewInputControlDemolish* pThis, OccupantFilterType type)
	{
		if (occupantFilterType != type)
		{
			occupantFilterType = type;

			switch (occupantFilterType)
			{
			case OccupantFilterType::Flora:
				pThis->SetCursor(cSC4ViewInputControlDemolishHooks::BulldozeCursorFlora);
				break;
			case OccupantFilterType::Network:
				pThis->SetCursor(cSC4ViewInputControlDemolishHooks::BulldozeCursorNetwork);
				break;
			case OccupantFilterType::None:
			default:
				pThis->SetCursor(cSC4ViewInputControlDemolishHooks::BulldozeCursorDefault);
				break;
			}

			if (pThis->bCellPicked)
			{
				UpdateSelectedRegion(pThis);
			}
		}
	}

	enum ModifierKeysFlags : int32_t
	{
		ModifierKeyFlagNone = 0,
		ModifierKeyFlagShift = 0x1,
		ModifierKeyFlagControl = 0x2,
		ModifierKeyFlagAlt = 0x4,
		ModifierKeyFlagAll = ModifierKeyFlagShift | ModifierKeyFlagControl | ModifierKeyFlagAlt,
	};

	bool __fastcall OnKeyDownHook(
		cSC4ViewInputControlDemolish* pThis,
		void* edxUnused,
		int32_t vkCode,
		int32_t modifiers)
	{
		bool handled = false;

		if (IsOnTop(pThis))
		{
			if (vkCode == VK_ESCAPE)
			{
				if (pThis->bCellPicked)
				{
					EndInput(pThis);
					handled = true;
				}
			}
			else
			{
				// Because we currently only have 2 additional bulldoze modes we
				// configure them using the B key with modifiers.
				// This also avoids any issues with overriding other tool shortcuts.
				if (vkCode == 'B')
				{
					handled = true;
					const uint32_t activeModifiers = modifiers & ModifierKeyFlagAll;

					if (activeModifiers == ModifierKeyFlagNone)
					{
						SetOccupantFilterOption(pThis, OccupantFilterType::None);
					}
					else if ((activeModifiers & ModifierKeyFlagControl) == ModifierKeyFlagControl)
					{
						SetOccupantFilterOption(pThis, OccupantFilterType::Flora);
					}
					else if ((activeModifiers & ModifierKeyFlagShift) == ModifierKeyFlagShift)
					{
						SetOccupantFilterOption(pThis, OccupantFilterType::Network);
					}
				}
			}
		}

		return handled;
	}

	void __fastcall Activate(cSC4ViewInputControlDemolish* pThis, void* edxUnused)
	{
		occupantFilterType = OccupantFilterType::None;

		switch (pThis->cursorIID)
		{
		case cSC4ViewInputControlDemolishHooks::BulldozeCursorFlora:
			occupantFilterType = OccupantFilterType::Flora;
			break;
		case cSC4ViewInputControlDemolishHooks::BulldozeCursorNetwork:
			occupantFilterType = OccupantFilterType::Network;
			break;
		}
	}

	bool DemolishRegion(
		cISC4Demolition* pDemolition,
		bool demolish,
		const SC4CellRegion<int32_t>& cellRegion,
		uint32_t privilegeType,
		uint32_t flags,
		bool clearZonedArea,
		int64_t* totalCost,
		intptr_t demolishedOccupantSet,
		cISC4Occupant* pDemolishEffectOccupant,
		long demolishEffectX,
		long demolishEffectZ)
	{
		cRZAutoRefCount<cISC4OccupantFilter> occupantFilter;

		switch (occupantFilterType)
		{
		case OccupantFilterType::Flora:
			occupantFilter = new FloraOccupantFilter();
			break;
		case OccupantFilterType::Network:
			occupantFilter = new NetworkOccupantFilter(NetworkTypeFlags::AllTransportationNetworks);
			break;
		case OccupantFilterType::None:
		default:
			break;
		}

		return pDemolition->DemolishRegion(
			demolish,
			cellRegion,
			privilegeType,
			flags,
			clearZonedArea,
			occupantFilter,
			totalCost,
			demolishedOccupantSet,
			pDemolishEffectOccupant,
			demolishEffectX,
			demolishEffectZ);
	}

	bool __fastcall UpdateSelectedRegionDemolishRegion(
		cISC4Demolition* pDemolition,
		void* edxUnused,
		SC4CellRegion<int32_t> const& cellRegion,
		intptr_t unused, // Originally the privilege type, but our patch overwrote it with a placeholder value.
		uint32_t flags,
		bool clearZonedArea,
		cISC4OccupantFilter* pOccupantFilter,
		int64_t* totalCost,
		intptr_t demolishedOccupantSet,
		cISC4Occupant* pDemolishEffectOccupant,
		long demolishEffectX,
		long demolishEffectZ)
	{
		return DemolishRegion(
			pDemolition,
			false, // demolish
			cellRegion,
			1, // privilegeType
			flags,
			clearZonedArea,
			totalCost,
			demolishedOccupantSet,
			pDemolishEffectOccupant,
			demolishEffectX,
			demolishEffectZ);
	}

	bool __fastcall OnMouseUpLDemolishRegion(
		cISC4Demolition* pDemolition,
		void* edxUnused,
		SC4CellRegion<int32_t> const& cellRegion,
		intptr_t unused, // Originally the privilege type, but our patch overwrote it with a placeholder value.
		uint32_t flags,
		bool clearZonedArea,
		cISC4OccupantFilter* pOccupantFilter,
		int64_t* totalCost,
		intptr_t demolishedOccupantSet,
		cISC4Occupant* pDemolishEffectOccupant,
		long demolishEffectX,
		long demolishEffectZ)
	{
		return DemolishRegion(
			pDemolition,
			true, // demolish
			cellRegion,
			1, // privilegeType
			flags,
			clearZonedArea,
			totalCost,
			demolishedOccupantSet,
			pDemolishEffectOccupant,
			demolishEffectX,
			demolishEffectZ);
	}

	void InstallUpdateSelectedRegionDemolishRegionHook()
	{
		// Original code:
		// 0x4b97ed-0x4b97ee = push 0x1
		// 0x4b97ef			 = push eax
		// 0x4b97f0			 = call dword ptr [EDX + 0x18]
		//
		// New code:
		// 0x4b97ed = push esi - padding to replace the push we overwrote
		// 0x4b97ee = push eax
		// 0x4b97ef = call <our hook>
		Patcher::OverwriteMemory(0x4b97ed, 0x56); // push esi
		Patcher::OverwriteMemory(0x4b97ee, 0x50); // push eax
		Patcher::InstallCallHook(0x4b97ef, reinterpret_cast<uintptr_t>(&UpdateSelectedRegionDemolishRegion));
	}

	void InstallOnMouseUpLDemolishRegionHook()
	{
		Patcher::InstallCallHook(0x4b9d02, reinterpret_cast<uintptr_t>(&OnMouseUpLDemolishRegion));
	}
}

cRZAutoRefCount<cISC4ViewInputControl> cSC4ViewInputControlDemolishHooks::CreateViewInputControl(BulldozeCursor cursor)
{
	cRZAutoRefCount<cISC4ViewInputControl> instance;

	cIGZAllocatorServicePtr pAllocatorService;

	if (pAllocatorService)
	{
		auto pControl = static_cast<cSC4ViewInputControlDemolish*>(pAllocatorService->Allocate(sizeof(cSC4ViewInputControlDemolish)));

		if (pControl)
		{
			cSC4ViewInputControlDemolish_ctor(pControl);

			instance = pControl;

			// We first call Init to let SC4 set its default cursor, then we set the correct one.
			instance->Init();
			instance->SetCursor(cursor);
		}
	}

	return instance;
}

bool cSC4ViewInputControlDemolishHooks::Install()
{
	bool installed = false;

	Logger& logger = Logger::GetInstance();
	const uint16_t gameVersion = SC4VersionDetection::GetInstance().GetGameVersion();

	if (gameVersion == 641)
	{
		try
		{
			Patcher::InstallJumpTableHook(0xa901d8, reinterpret_cast<uintptr_t>(&OnKeyDownHook));
			Patcher::InstallJumpTableHook(0xa901fc, reinterpret_cast<uintptr_t>(&Activate));
			InstallUpdateSelectedRegionDemolishRegionHook();
			InstallOnMouseUpLDemolishRegionHook();

			logger.WriteLine(LogLevel::Info, "Installed the bulldozer extensions.");
			installed = true;
		}
		catch (const wil::ResultException& e)
		{
			logger.WriteLineFormatted(
				LogLevel::Error,
				"Failed to install the bulldozer extensions.\n%s",
				e.what());
		}
	}
	else
	{
		logger.WriteLineFormatted(
			LogLevel::Error,
			"Unsupported game version: %d",
			gameVersion);
	}

	return installed;
}

