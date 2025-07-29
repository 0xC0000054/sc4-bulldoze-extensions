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

#include "cSC4ViewInputControlDemolishHooks.h"
#include "wil/result.h"
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
#include <cstdint>
#include <algorithm>
#include <cstdlib>

namespace
{
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
		uint8_t unknown4[64];
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
	static bool diagonalMode = false;
	static cSC4ViewInputControlDemolish* currentViewControl = nullptr;

	// Helper function to create a diagonal region from two points
	SC4CellRegion<int32_t> CreateDiagonalRegion(int32_t x1, int32_t z1, int32_t x2, int32_t z2)
	{
		Logger& logger = Logger::GetInstance();
		
		// Log the input rectangle coordinates
		logger.WriteLineFormatted(LogLevel::Debug, 
			"Creating diagonal region from rectangle: (%d,%d) to (%d,%d)", 
			x1, z1, x2, z2);

		// Calculate bounding box for the region
		int32_t minX = (std::min)(x1, x2);
		int32_t maxX = (std::max)(x1, x2);
		int32_t minZ = (std::min)(z1, z2);
		int32_t maxZ = (std::max)(z1, z2);

		// Create region with all cells initially false
		SC4CellRegion<int32_t> region(minX, minZ, maxX, maxZ, false);

		// Use Bresenham's line algorithm to mark diagonal cells
		int32_t dx = abs(x2 - x1);
		int32_t dz = abs(z2 - z1);
		int32_t sx = x1 < x2 ? 1 : -1;
		int32_t sz = z1 < z2 ? 1 : -1;
		int32_t err = dx - dz;

		logger.WriteLineFormatted(LogLevel::Debug, 
			"Bresenham parameters: dx=%d, dz=%d, sx=%d, sz=%d, initial_err=%d", 
			dx, dz, sx, sz, err);

		int32_t currentX = x1;
		int32_t currentZ = z1;
		int32_t tileCount = 0;

		while (true)
		{
			// Set the current cell to true in the region
			int32_t cellX = currentX - minX;
			int32_t cellZ = currentZ - minZ;
			if (cellX >= 0 && cellX < (maxX - minX + 1) && cellZ >= 0 && cellZ < (maxZ - minZ + 1))
			{
				region.cellMap.SetValue(cellX, cellZ, true);
				tileCount++;
				
				// Trace level logging for each tile (optional, very verbose)
				if (logger.IsEnabled(LogLevel::Trace))
				{
					logger.WriteLineFormatted(LogLevel::Trace, 
						"Marking tile: (%d,%d)", currentX, currentZ);
				}
			}

			if (currentX == x2 && currentZ == z2) break;

			int32_t e2 = 2 * err;
			if (e2 > -dz)
			{
				err -= dz;
				currentX += sx;
			}
			if (e2 < dx)
			{
				err += dx;
				currentZ += sz;
			}
		}

		logger.WriteLineFormatted(LogLevel::Debug, 
			"Diagonal line created: %d tiles from (%d,%d) to (%d,%d)", 
			tileCount, x1, z1, x2, z2);

		return region;
	}

	typedef bool(__thiscall* cSC4ViewInputControl_IsOnTop)(cISC4ViewInputControl* pThis);

	static const cSC4ViewInputControl_IsOnTop IsOnTop = reinterpret_cast<cSC4ViewInputControl_IsOnTop>(0x5fb190);

	typedef void(__thiscall* cSC4ViewInputControlDemolish_ThiscallFn)(cSC4ViewInputControlDemolish* pThis);

	static const cSC4ViewInputControlDemolish_ThiscallFn EndInput = reinterpret_cast<cSC4ViewInputControlDemolish_ThiscallFn>(0x4b9040);
	static const cSC4ViewInputControlDemolish_ThiscallFn UpdateSelectedRegion = reinterpret_cast<cSC4ViewInputControlDemolish_ThiscallFn>(0x4b93b0);


	void SetOccupantFilterOption(cSC4ViewInputControlDemolish* pThis, OccupantFilterType type, bool diagonal)
	{
		// Always store the current view control for use in other hooks
		currentViewControl = pThis;
		
		if (occupantFilterType != type || diagonalMode != diagonal)
		{
			occupantFilterType = type;
			diagonalMode = diagonal;

			// Log mode change
			Logger& logger = Logger::GetInstance();
			const char* filterTypeName = "Normal";
			switch (occupantFilterType)
			{
			case OccupantFilterType::Flora:
				filterTypeName = "Flora";
				break;
			case OccupantFilterType::Network:
				filterTypeName = "Network";
				break;
			}
			
			logger.WriteLineFormatted(LogLevel::Info, 
				"Bulldoze mode changed: %s%s", 
				diagonalMode ? "Diagonal " : "", 
				filterTypeName);

			// Set cursor based on occupant filter type and diagonal mode
			switch (occupantFilterType)
			{
			case OccupantFilterType::Flora:
				pThis->SetCursor(diagonalMode ? 
					cSC4ViewInputControlDemolishHooks::BulldozeCursorFloraDiagonal : 
					cSC4ViewInputControlDemolishHooks::BulldozeCursorFlora);
				break;
			case OccupantFilterType::Network:
				pThis->SetCursor(diagonalMode ? 
					cSC4ViewInputControlDemolishHooks::BulldozeCursorNetworkDiagonal : 
					cSC4ViewInputControlDemolishHooks::BulldozeCursorNetwork);
				break;
			case OccupantFilterType::None:
			default:
				pThis->SetCursor(diagonalMode ? 
					cSC4ViewInputControlDemolishHooks::BulldozeCursorDefaultDiagonal : 
					cSC4ViewInputControlDemolishHooks::BulldozeCursorDefault);
				break;
			}

			if (pThis->bCellPicked)
			{
				// REVERSE ENGINEERING SOLUTION: Safely modify existing pCellRegion contents
				if (diagonal && pThis->pCellRegion)
				{
					Logger& logger = Logger::GetInstance();
					
					// Get current rectangular bounds
					const auto& bounds = pThis->pCellRegion->bounds;
					
					logger.WriteLineFormatted(LogLevel::Debug,
						"Updating pCellRegion cellMap for diagonal preview: (%d,%d) to (%d,%d)",
						bounds.topLeftX, bounds.topLeftY, bounds.bottomRightX, bounds.bottomRightY);
					
					// Create diagonal region to get the cell pattern
					SC4CellRegion<int32_t> diagonalRegion = CreateDiagonalRegion(
						bounds.topLeftX, bounds.topLeftY,
						bounds.bottomRightX, bounds.bottomRightY
					);
					
					// SAFE APPROACH: Only modify the cellMap contents, not the structure
					// Copy diagonal pattern into existing cellMap without changing pointers
					auto& existingCellMap = pThis->pCellRegion->cellMap;
					const auto& diagonalCellMap = diagonalRegion.cellMap;
					
					// Calculate dimensions from bounds (both regions should have same bounds)
					const auto& existingBounds = pThis->pCellRegion->bounds;
					const auto& diagonalBounds = diagonalRegion.bounds;
					
					uint32_t width = static_cast<uint32_t>(bounds.bottomRightX - bounds.topLeftX + 1);
					uint32_t height = static_cast<uint32_t>(bounds.bottomRightY - bounds.topLeftY + 1);
					
					// Verify bounds match before copying
					if (existingBounds.topLeftX == diagonalBounds.topLeftX &&
						existingBounds.topLeftY == diagonalBounds.topLeftY &&
						existingBounds.bottomRightX == diagonalBounds.bottomRightX &&
						existingBounds.bottomRightY == diagonalBounds.bottomRightY)
					{
						// Copy cell values from diagonal pattern to existing map
						for (uint32_t x = 0; x < width; x++)
						{
							for (uint32_t z = 0; z < height; z++)
							{
								bool diagonalValue = diagonalCellMap.GetValue(x, z);
								existingCellMap.SetValue(x, z, diagonalValue);
							}
						}
						
						logger.WriteLineFormatted(LogLevel::Debug,
							"Successfully updated cellMap with diagonal pattern (%dx%d)", width, height);
					}
					else
					{
						logger.WriteLineFormatted(LogLevel::Error,
							"CellMap bounds mismatch - skipping diagonal update");
					}
				}
				
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
				// Configure bulldoze modes using the B key with modifiers.
				// Alt acts as a diagonal modifier on top of the base modes.
				if (vkCode == 'B')
				{
					handled = true;
					const uint32_t activeModifiers = modifiers & ModifierKeyFlagAll;
					const bool isDiagonal = (activeModifiers & ModifierKeyFlagAlt) == ModifierKeyFlagAlt;

					if (activeModifiers == ModifierKeyFlagNone)
					{
						// B - Normal bulldoze
						SetOccupantFilterOption(pThis, OccupantFilterType::None, false);
					}
					else if (activeModifiers == ModifierKeyFlagAlt)
					{
						// Alt+B - Diagonal normal bulldoze
						SetOccupantFilterOption(pThis, OccupantFilterType::None, true);
					}
					else if ((activeModifiers & ModifierKeyFlagControl) == ModifierKeyFlagControl)
					{
						// Ctrl+B or Alt+Ctrl+B - Flora bulldoze (with or without diagonal)
						SetOccupantFilterOption(pThis, OccupantFilterType::Flora, isDiagonal);
					}
					else if ((activeModifiers & ModifierKeyFlagShift) == ModifierKeyFlagShift)
					{
						// Shift+B or Alt+Shift+B - Network bulldoze (with or without diagonal)
						SetOccupantFilterOption(pThis, OccupantFilterType::Network, isDiagonal);
					}
				}
			}
		}

		return handled;
	}

	void __fastcall Activate(cSC4ViewInputControlDemolish* pThis, void* edxUnused)
	{
		occupantFilterType = OccupantFilterType::None;
		diagonalMode = false;
		currentViewControl = pThis;

		switch (pThis->cursorIID)
		{
		case cSC4ViewInputControlDemolishHooks::BulldozeCursorFlora:
			occupantFilterType = OccupantFilterType::Flora;
			break;
		case cSC4ViewInputControlDemolishHooks::BulldozeCursorFloraDiagonal:
			occupantFilterType = OccupantFilterType::Flora;
			diagonalMode = true;
			break;
		case cSC4ViewInputControlDemolishHooks::BulldozeCursorNetwork:
			occupantFilterType = OccupantFilterType::Network;
			break;
		case cSC4ViewInputControlDemolishHooks::BulldozeCursorNetworkDiagonal:
			occupantFilterType = OccupantFilterType::Network;
			diagonalMode = true;
			break;
		case cSC4ViewInputControlDemolishHooks::BulldozeCursorDefaultDiagonal:
			diagonalMode = true;
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
		Logger& logger = Logger::GetInstance();
		
		// REVERSE ENGINEERING SOLUTION: Use the globally stored view control pointer
		// This is much more reliable than stack walking or register inspection
		
		// Apply diagonal modification if enabled and we have valid view control
		if (diagonalMode && currentViewControl && currentViewControl->pCellRegion)
		{
			cSC4ViewInputControlDemolish* pViewControl = currentViewControl;
			const auto& bounds = cellRegion.bounds;
			logger.WriteLineFormatted(LogLevel::Info, 
				"Diagonal bulldoze preview: original region (%d,%d) to (%d,%d)", 
				bounds.topLeftX, bounds.topLeftY, bounds.bottomRightX, bounds.bottomRightY);
			
			// Create diagonal region
			SC4CellRegion<int32_t> diagonalRegion = CreateDiagonalRegion(
				bounds.topLeftX, bounds.topLeftY,
				bounds.bottomRightX, bounds.bottomRightY
			);
			
			// SAFE APPROACH: Update view control's cellMap contents without changing structure
			auto& existingCellMap = pViewControl->pCellRegion->cellMap;
			const auto& diagonalCellMap = diagonalRegion.cellMap;
			
			// Calculate dimensions from bounds
			const auto& existingBounds = pViewControl->pCellRegion->bounds;
			const auto& diagonalBounds = diagonalRegion.bounds;
			
			uint32_t width = static_cast<uint32_t>(bounds.bottomRightX - bounds.topLeftX + 1);
			uint32_t height = static_cast<uint32_t>(bounds.bottomRightY - bounds.topLeftY + 1);
			
			// Verify bounds match and update cellMap safely
			if (existingBounds.topLeftX == diagonalBounds.topLeftX &&
				existingBounds.topLeftY == diagonalBounds.topLeftY &&
				existingBounds.bottomRightX == diagonalBounds.bottomRightX &&
				existingBounds.bottomRightY == diagonalBounds.bottomRightY)
			{
				// Copy diagonal pattern into existing cellMap
				for (uint32_t x = 0; x < width; x++)
				{
					for (uint32_t z = 0; z < height; z++)
					{
						bool diagonalValue = diagonalCellMap.GetValue(x, z);
						existingCellMap.SetValue(x, z, diagonalValue);
					}
				}
				
				logger.WriteLineFormatted(LogLevel::Debug, 
					"Updated view control cellMap with diagonal pattern for preview (%dx%d)", width, height);
			}
			
			// Call demolish with diagonal region for preview calculation
			bool result = DemolishRegion(
				pDemolition,
				false, // demolish
				diagonalRegion,
				1, // privilegeType
				flags,
				clearZonedArea,
				totalCost,
				demolishedOccupantSet,
				pDemolishEffectOccupant,
				demolishEffectX,
				demolishEffectZ);
			
			return result;
		}

		// Normal rectangular bulldoze preview
		const auto& bounds = cellRegion.bounds;
		int32_t tileCount = (bounds.bottomRightX - bounds.topLeftX + 1) * (bounds.bottomRightY - bounds.topLeftY + 1);
		logger.WriteLineFormatted(LogLevel::Debug, 
			"Normal bulldoze preview: %d tiles in region (%d,%d) to (%d,%d)", 
			tileCount, bounds.topLeftX, bounds.topLeftY, bounds.bottomRightX, bounds.bottomRightY);

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
		Logger& logger = Logger::GetInstance();
		
		// Apply diagonal modification if enabled
		if (diagonalMode)
		{
			const auto& bounds = cellRegion.bounds;
			logger.WriteLineFormatted(LogLevel::Info, 
				"EXECUTING diagonal bulldoze: original region (%d,%d) to (%d,%d)", 
				bounds.topLeftX, bounds.topLeftY, bounds.bottomRightX, bounds.bottomRightY);
			
			SC4CellRegion<int32_t> diagonalRegion = CreateDiagonalRegion(
				bounds.topLeftX, bounds.topLeftY,
				bounds.bottomRightX, bounds.bottomRightY
			);
			
			bool result = DemolishRegion(
				pDemolition,
				true, // demolish
				diagonalRegion,
				1, // privilegeType
				flags,
				clearZonedArea,
				totalCost,
				demolishedOccupantSet,
				pDemolishEffectOccupant,
				demolishEffectX,
				demolishEffectZ);
			
			logger.WriteLineFormatted(LogLevel::Info, 
				"Diagonal bulldoze completed: success=%s", result ? "true" : "false");
			
			return result;
		}

		// Normal rectangular bulldoze execution
		const auto& bounds = cellRegion.bounds;
		int32_t tileCount = (bounds.bottomRightX - bounds.topLeftX + 1) * (bounds.bottomRightY - bounds.topLeftY + 1);
		logger.WriteLineFormatted(LogLevel::Debug, 
			"EXECUTING normal bulldoze: %d tiles in region (%d,%d) to (%d,%d)", 
			tileCount, bounds.topLeftX, bounds.topLeftY, bounds.bottomRightX, bounds.bottomRightY);

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

