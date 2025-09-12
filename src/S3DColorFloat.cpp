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

#include "S3DColorFloat.h"

S3DColorFloat::S3DColorFloat()
	: r(1.0), g(1.0), b(1.0), a(1.0)
{
}

S3DColorFloat::S3DColorFloat(float red, float green, float blue, float alpha)
	: r(red), g(green), b(blue), a(alpha)
{
}

S3DColorFloat::S3DColorFloat(S3DColorFloat const& other)
	: r(other.r), g(other.g), b(other.b), a(other.a)
{
}

S3DColorFloat::S3DColorFloat(S3DColorFloat&& other) noexcept
	: r(other.r), g(other.g), b(other.b), a(other.a)
{
}

S3DColorFloat& S3DColorFloat::operator=(S3DColorFloat const& other)
{
	r = other.r;
	g = other.g;
	b = other.b;
	a = other.a;

	return *this;
}

S3DColorFloat& S3DColorFloat::operator=(S3DColorFloat&& other) noexcept
{
	r = other.r;
	g = other.g;
	b = other.b;
	a = other.a;

	return *this;
}