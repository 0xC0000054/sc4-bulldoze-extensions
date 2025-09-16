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

struct S3DColorFloat
{
	float r;
	float g;
	float b;
	float a;

	S3DColorFloat();
	S3DColorFloat(float red, float green, float blue, float alpha);

	S3DColorFloat(S3DColorFloat const& other);
	S3DColorFloat(S3DColorFloat&& other) noexcept;

	S3DColorFloat& operator=(S3DColorFloat const& other);
	S3DColorFloat& operator=(S3DColorFloat&& other) noexcept;
};