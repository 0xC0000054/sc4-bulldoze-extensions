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
	static bool diagonalMode = false;
	static int32_t diagonalThickness = 1; // Default thickness is 1 (single line)
	static cSC4ViewInputControlDemolish* currentViewControl = nullptr;


	// Helper function to create a diagonal region from two points with drag direction detection and thickness
	SC4CellRegion<int32_t> CreateDiagonalRegion(int32_t x1, int32_t z1, int32_t x2, int32_t z2, int32_t startX = -1, int32_t startZ = -1)
	{
		Logger& logger = Logger::GetInstance();

		// LOG: Diagnostic information about cell points
		logger.WriteLineFormatted(LogLevel::Debug,
			"CreateDiagonalRegion: bounds=(%d,%d) to (%d,%d), startPoint=(%d,%d)",
			x1, z1, x2, z2, startX, startZ);

		// Calculate bounding box for the region
		int32_t minX = (std::min)(x1, x2);
		int32_t maxX = (std::max)(x1, x2);
		int32_t minZ = (std::min)(z1, z2);
		int32_t maxZ = (std::max)(z1, z2);

		// Create region with all cells initially false
		SC4CellRegion<int32_t> region(minX, minZ, maxX, maxZ, false);

		// STABLE ENDPOINT CALCULATION: Use the original drag coordinates (x1,z1) and (x2,z2)
		// instead of always using bounds corners. This prevents jumping during drag operations.
		int32_t diagStartX, diagStartZ, diagEndX, diagEndZ;
		
		if (startX != -1 && startZ != -1)
		{
			// Handle start points that might be outside bounds
			// This can happen when the drag starts on one cell but the final bounds exclude it for some reason. Fixes issues in built-up cities.
			
			// First, try exact corner matching
			if (startX == minX && startZ == minZ)
			{
				// Started from top-left -> draw to bottom-right
				diagStartX = minX; diagStartZ = minZ;
				diagEndX = maxX; diagEndZ = maxZ;
				logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Exact match TL->BR");
			}
			else if (startX == maxX && startZ == minZ)
			{
				// Started from top-right -> draw to bottom-left  
				diagStartX = maxX; diagStartZ = minZ;
				diagEndX = minX; diagEndZ = maxZ;
				logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Exact match TR->BL");
			}
			else if (startX == minX && startZ == maxZ)
			{
				// Started from bottom-left -> draw to top-right
				diagStartX = minX; diagStartZ = maxZ;
				diagEndX = maxX; diagEndZ = minZ;
				logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Exact match BL->TR");
			}
			else if (startX == maxX && startZ == maxZ)
			{
				// Started from bottom-right -> draw to top-left
				diagStartX = maxX; diagStartZ = maxZ;
				diagEndX = minX; diagEndZ = minZ;
				logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Exact match BR->TL");
			}
			else
			{
				// Start point doesn't match corners exactly
				// Try to infer direction based on which side of bounds the start point is on
				logger.WriteLineFormatted(LogLevel::Debug,
					"CreateDiagonalRegion: Start point (%d,%d) outside bounds (%d,%d) to (%d,%d), inferring direction",
					startX, startZ, minX, minZ, maxX, maxZ);
				
				// Determine which quadrant/edge relative to bounds the start point is in
				bool startIsLeft = startX < minX;      // Strictly less than (outside left edge)
				bool startIsTop = startZ < minZ;       // Strictly less than (outside top edge)  
				bool startIsRight = startX > maxX;     // Strictly greater than (outside right edge)
				bool startIsBottom = startZ > maxZ;    // Strictly greater than (outside bottom edge)
				bool startOnLeftEdge = startX == minX;  // Exactly on left edge
				bool startOnTopEdge = startZ == minZ;   // Exactly on top edge
				bool startOnRightEdge = startX == maxX; // Exactly on right edge
				bool startOnBottomEdge = startZ == maxZ; // Exactly on bottom edge
				
				if (startIsLeft && startIsTop)
				{
					// Start is northwest of bounds -> diagonal TL to BR
					diagStartX = minX; diagStartZ = minZ;
					diagEndX = maxX; diagEndZ = maxZ;
					logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inferred NW quadrant->SE (TL->BR)");
				}
				else if (startIsRight && startIsTop)
				{
					// Start is northeast of bounds -> diagonal TR to BL
					diagStartX = maxX; diagStartZ = minZ;
					diagEndX = minX; diagEndZ = maxZ;
					logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inferred NE quadrant->SW (TR->BL)");
				}
				else if (startIsLeft && startIsBottom)
				{
					// Start is southwest of bounds -> diagonal BL to TR
					diagStartX = minX; diagStartZ = maxZ;
					diagEndX = maxX; diagEndZ = minZ;
					logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inferred SW quadrant->NE (BL->TR)");
				}
				else if (startIsRight && startIsBottom)
				{
					// Start is southeast of bounds -> diagonal BR to TL
					diagStartX = maxX; diagStartZ = maxZ;
					diagEndX = minX; diagEndZ = minZ;
					logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inferred SE quadrant->NW (BR->TL)");
				}
				// Handle edge cases where start point is on an edge or inside bounds
				else if (startOnLeftEdge || startIsLeft)
				{
					// On or near left edge - determine vertical direction
					if (startZ <= minZ + (maxZ - minZ) / 2)
					{
						// Closer to top -> TL to BR
						diagStartX = minX; diagStartZ = minZ;
						diagEndX = maxX; diagEndZ = maxZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Left edge, upper half->SE (TL->BR)");
					}
					else
					{
						// Closer to bottom -> BL to TR
						diagStartX = minX; diagStartZ = maxZ;
						diagEndX = maxX; diagEndZ = minZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Left edge, lower half->NE (BL->TR)");
					}
				}
				else if (startOnRightEdge || startIsRight)
				{
					// On or near right edge - determine vertical direction
					if (startZ <= minZ + (maxZ - minZ) / 2)
					{
						// Closer to top -> TR to BL
						diagStartX = maxX; diagStartZ = minZ;
						diagEndX = minX; diagEndZ = maxZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Right edge, upper half->SW (TR->BL)");
					}
					else
					{
						// Closer to bottom -> BR to TL
						diagStartX = maxX; diagStartZ = maxZ;  
						diagEndX = minX; diagEndZ = minZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Right edge, lower half->NW (BR->TL)");
					}
				}
				else if (startOnTopEdge || startIsTop)
				{
					// On or near top edge - determine horizontal direction
					if (startX <= minX + (maxX - minX) / 2)
					{
						// Closer to left -> TL to BR
						diagStartX = minX; diagStartZ = minZ;
						diagEndX = maxX; diagEndZ = maxZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Top edge, left half->SE (TL->BR)");
					}
					else
					{
						// Closer to right -> TR to BL
						diagStartX = maxX; diagStartZ = minZ;
						diagEndX = minX; diagEndZ = maxZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Top edge, right half->SW (TR->BL)");
					}
				}
				else if (startOnBottomEdge || startIsBottom)
				{
					// On or near bottom edge - determine horizontal direction
					if (startX <= minX + (maxX - minX) / 2)
					{
						// Closer to left -> BL to TR
						diagStartX = minX; diagStartZ = maxZ;
						diagEndX = maxX; diagEndZ = minZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Bottom edge, left half->NE (BL->TR)");
					}
					else
					{
						// Closer to right -> BR to TL
						diagStartX = maxX; diagStartZ = maxZ;
						diagEndX = minX; diagEndZ = minZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Bottom edge, right half->NW (BR->TL)");
					}
				}
				else
				{
					// Inside bounds or other edge case - use position relative to center
					int32_t centerX = minX + (maxX - minX) / 2;
					int32_t centerZ = minZ + (maxZ - minZ) / 2;
					
					if (startX <= centerX && startZ <= centerZ)
					{
						// Northwest quadrant within bounds -> TL to BR
						diagStartX = minX; diagStartZ = minZ;
						diagEndX = maxX; diagEndZ = maxZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inside bounds, NW->SE (TL->BR)");
					}
					else if (startX > centerX && startZ <= centerZ)
					{
						// Northeast quadrant within bounds -> TR to BL
						diagStartX = maxX; diagStartZ = minZ;
						diagEndX = minX; diagEndZ = maxZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inside bounds, NE->SW (TR->BL)");
					}
					else if (startX <= centerX && startZ > centerZ)
					{
						// Southwest quadrant within bounds -> BL to TR
						diagStartX = minX; diagStartZ = maxZ;
						diagEndX = maxX; diagEndZ = minZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inside bounds, SW->NE (BL->TR)");
					}
					else
					{
						// Southeast quadrant within bounds -> BR to TL
						diagStartX = maxX; diagStartZ = maxZ;
						diagEndX = minX; diagEndZ = minZ;
						logger.WriteLineFormatted(LogLevel::Debug, "CreateDiagonalRegion: Inside bounds, SE->NW (BR->TL)");
					}
				}
			}
		}
		else
		{
			// No start point provided - use default NW-to-SE diagonal
			logger.WriteLineFormatted(LogLevel::Debug,
				"CreateDiagonalRegion: No start point provided, defaulting to NW-SE diagonal");
			diagStartX = minX; diagStartZ = minZ;
			diagEndX = maxX; diagEndZ = maxZ;
		}

		// Use Bresenham's line algorithm to mark diagonal cells
		int32_t dx = abs(diagEndX - diagStartX);
		int32_t dz = abs(diagEndZ - diagStartZ);
		int32_t sx = diagStartX < diagEndX ? 1 : -1;
		int32_t sz = diagStartZ < diagEndZ ? 1 : -1;
		int32_t err = dx - dz;


		int32_t currentX = diagStartX;
		int32_t currentZ = diagStartZ;
		int32_t tileCount = 0;

		while (true)
		{
			// Set the current cell and perpendicular cells for thickness
			// Handle positive/negative thickness values (skip 0)
			int32_t startOffset, endOffset;
			if (diagonalThickness > 0) {
				startOffset = 0;
				endOffset = diagonalThickness - 1;
			} else {
				startOffset = diagonalThickness + 1;
				endOffset = 0;
			}
			
			for (int32_t thickOffset = startOffset; thickOffset <= endOffset; thickOffset++)
			{
				// Calculate perpendicular offset based on line direction
				int32_t perpX, perpZ;
				if (abs(dx) > abs(dz)) {
					// More horizontal line - add thickness vertically
					perpX = currentX;
					perpZ = currentZ + thickOffset;
				} else {
					// More vertical line - add thickness horizontally  
					perpX = currentX + thickOffset;
					perpZ = currentZ;
				}
				
				int32_t cellX = perpX - minX;
				int32_t cellZ = perpZ - minZ;
				if (cellX >= 0 && cellX < (maxX - minX + 1) && cellZ >= 0 && cellZ < (maxZ - minZ + 1))
				{
					region.cellMap.SetValue(cellX, cellZ, true);
					tileCount++;
				}
			}

			if (currentX == diagEndX && currentZ == diagEndZ) break;

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
			"Created diagonal line: %d tiles, thickness=%d", tileCount, diagonalThickness);

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
				// Safely modify existing pCellRegion contents
				if (diagonal && pThis->pCellRegion)
				{
					// Get current rectangular bounds
					const auto& bounds = pThis->pCellRegion->bounds;
					
					// LOG: Diagnostic information about cell points and unknown2 array
					Logger& logger = Logger::GetInstance();
					logger.WriteLineFormatted(LogLevel::Debug,
						"SetOccupantFilterOption: cellPointX=%d, cellPointZ=%d, bCellPicked=%d",
						pThis->cellPointX, pThis->cellPointZ, pThis->bCellPicked);

					// Log unknown2 array contents when cell points might be invalid
					if (pThis->cellPointX == -1 || pThis->cellPointZ == -1 || 
						pThis->cellPointX == 0 || pThis->cellPointZ == 0)
					{
						std::string unknown2Hex;
						for (int i = 0; i < 27; i++)
						{
							char hexBuf[4];
							sprintf_s(hexBuf, "%02X", pThis->unknown2[i]);
							if (i > 0) unknown2Hex += " ";
							unknown2Hex += hexBuf;
						}
						logger.WriteLineFormatted(LogLevel::Debug,
							"SetOccupantFilterOption: Cell points invalid, unknown2[27] = %s",
							unknown2Hex.c_str());
					}

					// Create diagonal region to get the cell pattern
					// Pass the actual drag start point from the view control
					SC4CellRegion<int32_t> diagonalRegion = CreateDiagonalRegion(
						bounds.topLeftX, bounds.topLeftY,
						bounds.bottomRightX, bounds.bottomRightY,
						pThis->cellPointX, pThis->cellPointZ
					);
					
					// Only modify the cellMap contents, not the structure
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

	bool __fastcall OnMouseWheelHook(
		cSC4ViewInputControlDemolish* pThis,
		void* edxUnused,
		int32_t x,
		int32_t z,
		uint32_t modifiers,
		int32_t wheelDelta)
	{
		Logger& logger = Logger::GetInstance();
		
		// Check if we're in diagonal mode and Alt is held
		if (diagonalMode && (modifiers & ModifierKeyFlagAlt))
		{
			// Adjust diagonal thickness based on wheel direction
			int32_t oldThickness = diagonalThickness;
			
			if (wheelDelta > 0)
			{
				// Scroll up - increase thickness (max 5, skip 0)
				if (diagonalThickness == -1) diagonalThickness = 1;
				else diagonalThickness = (std::min)(diagonalThickness + 1, 5);
			}
			else if (wheelDelta < 0)
			{
				// Scroll down - decrease thickness (min -5, skip 0)
				if (diagonalThickness == 1) diagonalThickness = -1;
				else diagonalThickness = (std::max)(diagonalThickness - 1, -5);
			}
			
			if (diagonalThickness != oldThickness)
			{
				logger.WriteLineFormatted(LogLevel::Debug,
					"Diagonal thickness: %d -> %d", oldThickness, diagonalThickness);
				
				// Update preview if we have an active selection
				if (pThis->bCellPicked && pThis->pCellRegion)
				{
					// Trigger preview update by calling SetOccupantFilterOption
					SetOccupantFilterOption(pThis, occupantFilterType, diagonalMode);
					
					// Force immediate visual update of the preview
					UpdateSelectedRegion(pThis);
				}
			}
			
			// Return true to indicate we handled the event (prevents zooming)
			return true;
		}
		
		// Let default behavior handle normal zoom
		return false;
	}

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
		diagonalThickness = 1; // Reset thickness to default
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
		
		// Try to modify preview colors using our global view control pointer
		if (currentViewControl)
		{
			// Intuitive color scheme with alpha blending:
			// RGBA format: [0]=Red, [1]=Green, [2]=Blue, [3]=Alpha
			float floraColor[4] = { 0.38f, 0.69f, 0.38f, 0.5f };   // Green for flora/nature
			float networkColor[4] = { 0.98f, 0.60f, 0.20f, 0.5f }; // Orange for networks/infrastructure
			
			// Define blue color for standard bulldoze (classic demolish color)
			float normalColor[4] = { 0.30f, 0.60f, 0.85f, 0.5f };   // Blue for standard
			
			float* colorToUse = nullptr;
			const char* modeName = "Normal";
			
			switch (occupantFilterType)
			{
			case OccupantFilterType::Flora:
				colorToUse = floraColor;
				modeName = "Flora";
				break;
			case OccupantFilterType::Network:
				colorToUse = networkColor;
				modeName = "Network";
				break;
			case OccupantFilterType::None:
			default:
				colorToUse = normalColor;
				modeName = "Normal";
				break;
			}
			
			if (colorToUse != nullptr)
			{
				// Direct access to demolishOK (main preview color array)
				S3DColorFloat* previewColor = &(currentViewControl->demolishOK);
				
				// Debug logging: show before and after colors
				logger.WriteLineFormatted(LogLevel::Trace,
					"Preview color BEFORE %s: R=%.3f, G=%.3f, B=%.3f, A=%.3f",
					modeName, previewColor->r, previewColor->g, previewColor->b, previewColor->a);

				// Set preview color
				previewColor->r = colorToUse[0]; // Red
				previewColor->g = colorToUse[1]; // Green
				previewColor->b = colorToUse[2]; // Blue
				previewColor->a = colorToUse[3]; // Alpha
				
				logger.WriteLineFormatted(LogLevel::Trace, 
					"Preview color AFTER %s: R=%.3f, G=%.3f, B=%.3f, A=%.3f", 
					modeName, previewColor->r, previewColor->g, previewColor->b, previewColor->a);
			}
		}
		
		// Apply diagonal modification if enabled and we have valid view control
		if (diagonalMode && currentViewControl && currentViewControl->pCellRegion)
		{
			cSC4ViewInputControlDemolish* pViewControl = currentViewControl;
			const auto& bounds = cellRegion.bounds;
			
			// LOG: Diagnostic information for preview function
			logger.WriteLineFormatted(LogLevel::Debug,
				"DemolishPreview: cellPointX=%d, cellPointZ=%d, bCellPicked=%d",
				pViewControl->cellPointX, pViewControl->cellPointZ, pViewControl->bCellPicked);
			
			// Log unknown2 array contents when cell points might be invalid
			if (pViewControl->cellPointX == -1 || pViewControl->cellPointZ == -1 || 
				pViewControl->cellPointX == 0 || pViewControl->cellPointZ == 0)
			{
				std::string unknown2Hex;
				for (int i = 0; i < 27; i++)
				{
					char hexBuf[4];
					sprintf_s(hexBuf, "%02X", pViewControl->unknown2[i]);
					if (i > 0) unknown2Hex += " ";
					unknown2Hex += hexBuf;
				}
				logger.WriteLineFormatted(LogLevel::Debug,
					"DemolishPreview: Cell points invalid, unknown2[27] = %s",
					unknown2Hex.c_str());
			}
			
			// Create diagonal region
			// Pass the actual drag start point from the view control
			SC4CellRegion<int32_t> diagonalRegion = CreateDiagonalRegion(
				bounds.topLeftX, bounds.topLeftY,
				bounds.bottomRightX, bounds.bottomRightY,
				pViewControl->cellPointX, pViewControl->cellPointZ
			);
			
			// Update view control's cellMap contents without changing structure
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
			
			// LOG: Diagnostic information for actual demolish function
			if (currentViewControl)
			{
				logger.WriteLineFormatted(LogLevel::Debug,
					"OnMouseUpLDemolishRegion: cellPointX=%d, cellPointZ=%d, bCellPicked=%d",
					currentViewControl->cellPointX, currentViewControl->cellPointZ, currentViewControl->bCellPicked);
				
				// Log unknown2 array contents when cell points might be invalid
				if (currentViewControl->cellPointX == -1 || currentViewControl->cellPointZ == -1 || 
					currentViewControl->cellPointX == 0 || currentViewControl->cellPointZ == 0)
				{
					std::string unknown2Hex;
					for (int i = 0; i < 27; i++)
					{
						char hexBuf[4];
						sprintf_s(hexBuf, "%02X", currentViewControl->unknown2[i]);
						if (i > 0) unknown2Hex += " ";
						unknown2Hex += hexBuf;
					}
					logger.WriteLineFormatted(LogLevel::Debug,
						"OnMouseUpLDemolishRegion: Cell points invalid, unknown2[27] = %s",
						unknown2Hex.c_str());
				}
			}
			else
			{
				logger.WriteLineFormatted(LogLevel::Debug,
					"OnMouseUpLDemolishRegion: currentViewControl is null!");
			}
			
			// Pass the actual drag start point from the view control
			SC4CellRegion<int32_t> diagonalRegion = CreateDiagonalRegion(
				bounds.topLeftX, bounds.topLeftY,
				bounds.bottomRightX, bounds.bottomRightY,
				currentViewControl ? currentViewControl->cellPointX : -1,
				currentViewControl ? currentViewControl->cellPointZ : -1
			);
			
			return DemolishRegion(
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
		}

		// Normal rectangular bulldoze execution

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
			Patcher::InstallJumpTableHook(0xa901f4, reinterpret_cast<uintptr_t>(&OnMouseWheelHook));
			Patcher::InstallJumpTableHook(0xa901fc, reinterpret_cast<uintptr_t>(&Activate));
			InstallUpdateSelectedRegionDemolishRegionHook();
			InstallOnMouseUpLDemolishRegionHook();

			logger.WriteLine(LogLevel::Info, "Installed the bulldozer extensions (with preview colors).");
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

