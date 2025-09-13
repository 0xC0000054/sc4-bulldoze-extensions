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

#include "BulldozeHighlightColors.h"
#include "cGZPersistResourceKey.h"
#include "cIGZPersistResourceManager.h"
#include "cIGZVariant.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cISCResExemplar.h"
#include "cRZAutoRefCount.h"
#include "GZServPtrs.h"
#include "Logger.h"

namespace
{
	void SetColorFromProperty(const cISCPropertyHolder* pPropertyHolder, uint32_t propertyID, S3DColorFloat& color, bool logError = true)
	{
		if (pPropertyHolder)
		{
			const cISCProperty* pProperty = pPropertyHolder->GetProperty(propertyID);

			if (pProperty)
			{
				const cIGZVariant* pVariant = pProperty->GetPropertyValue();

				if (pVariant)
				{
					if (pVariant->GetType() == cIGZVariant::Float32Array
						&& pVariant->GetCount() == 4)
					{
						const float* pData = pVariant->RefFloat32();

						color.r = pData[0];
						color.g = pData[1];
						color.b = pData[2];
						color.a = pData[3];
					}
					else if (logError)
					{
						Logger::GetInstance().WriteLineFormatted(
							LogLevel::Error,
							"Bulldoze Extensions Tuning Exemplar property 0x%08X must be a 4 item Float32 array.",
							propertyID);
					}
				}
			}
		}
	}

	S3DColorFloat GetDefaultDemolishOKColor()
	{
		// The default green 'Demolish OK' color SC4 uses (RGBA 0, 179, 51, 77).

		S3DColorFloat color(0.0F, 0.7F, 0.2F, 0.3F);

		cIGZPersistResourceManagerPtr pRM;

		if (pRM)
		{
			// The 'Demolish OK' and 'Demolish Not OK' colors can be overridden using
			// undocumented properties in the 'Model highlight properties' exemplar.
			// 'Demolish OK' uses property id 0xea639fba and 'Demolish Not OK' uses
			// property id id0xea639fbb.

			const cGZPersistResourceKey modelHighlightExemplarKey(0x6534284A, 0x690F693F, 0x4A639EF2);

			cRZAutoRefCount<cISCResExemplar> pExemplar;

			if (pRM->GetResource(
				modelHighlightExemplarKey,
				GZIID_cISCResExemplar,
				pExemplar.AsPPVoid(),
				0,
				nullptr))
			{
				constexpr uint32_t kDemolishOKPropertyID = 0xEA639FBA;

				SetColorFromProperty(
					pExemplar->AsISCPropertyHolder(),
					kDemolishOKPropertyID,
					color,
					false);
			}
		}

		return color;
	}
}

BulldozeHighlightColors::BulldozeHighlightColors()
	// The colors are initialized to zero in the constructor, the
	// correct default values will be set in Init.
	// This is done because GetDefaultDemolishOKColor requires the resource manager,
	// and it is unavailable at this stage in the DLL's startup process.
	: normalBulldozeColor(),
	  floraBulldozeHighlightColor(),
	  networkBulldozeHighlightColor(),
	  gameDefaultDemolishOKColor(),
	  initialized(false)
{
}

void BulldozeHighlightColors::Init()
{
	if (!initialized)
	{
		initialized = true;

		gameDefaultDemolishOKColor = GetDefaultDemolishOKColor();
		normalBulldozeColor = gameDefaultDemolishOKColor;
		floraBulldozeHighlightColor = gameDefaultDemolishOKColor;
		networkBulldozeHighlightColor = gameDefaultDemolishOKColor;

		cIGZPersistResourceManagerPtr pRM;

		if (pRM)
		{
			const cGZPersistResourceKey tuningExemplarKey(0x6534284A, 0xF527AC8F, 0x89EB3FF3);

			cRZAutoRefCount<cISCResExemplar> pExemplar;

			if (pRM->GetResource(tuningExemplarKey, GZIID_cISCResExemplar, pExemplar.AsPPVoid(), 0, nullptr))
			{
				constexpr uint32_t kNormalBulldozeHighlightColorPropertyID = 0x8FD94ED0;
				constexpr uint32_t kFloraBulldozeHighlightColorPropertyID = 0x8FD94ED1;
				constexpr uint32_t kNetworkBulldozeHighlightColorPropertyID = 0x8FD94ED2;

				const cISCPropertyHolder* pPropertyHolder = pExemplar->AsISCPropertyHolder();

				SetColorFromProperty(
					pPropertyHolder,
					kNormalBulldozeHighlightColorPropertyID,
					normalBulldozeColor);
				SetColorFromProperty(
					pPropertyHolder,
					kFloraBulldozeHighlightColorPropertyID,
					floraBulldozeHighlightColor);
				SetColorFromProperty(
					pPropertyHolder,
					kNetworkBulldozeHighlightColorPropertyID,
					networkBulldozeHighlightColor);
			}
		}
	}
}

void BulldozeHighlightColors::Shutdown()
{
	if (initialized)
	{
		initialized = false;
	}
}

const S3DColorFloat& BulldozeHighlightColors::GetDemolishOKColor(ColorType type) const
{
	switch (type)
	{
	case IBulldozeHighlightColors::ColorType::Normal:
		return normalBulldozeColor;
	case IBulldozeHighlightColors::ColorType::Flora:
		return floraBulldozeHighlightColor;
	case IBulldozeHighlightColors::ColorType::Network:
		return networkBulldozeHighlightColor;
	default:
		Logger::GetInstance().WriteLineFormatted(
			LogLevel::Error,
			"Unsupported HighlightColorType value %d. Using the default color.",
			static_cast<int32_t>(type));
		return gameDefaultDemolishOKColor;
	}
}