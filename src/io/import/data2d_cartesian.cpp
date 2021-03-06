/*
	*** Import - Cartesian Data2D
	*** src/io/import/data2d_cartesian.cpp
	Copyright T. Youngs 2012-2020

	This file is part of Dissolve.

	Dissolve is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Dissolve is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Dissolve.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "io/import/data2d.h"
#include "math/data2d.h"
#include "math/filters.h"
#include "base/lineparser.h"

// Read cartesian data from specified file
bool Data2DImportFileFormat::importCartesian(LineParser& parser, Data2D& data)
{
	/*
	 * Cartesian coordinates assume that there are three values per line: x, y, and value.
	 * The x and y values are assumed to be centre-bin values.
	 */

	// Must have axis ranges - otherwise tricky to work out axis limits without reading the whole file in twice
	if ((!keywords_.isSet("XRange")) || (!keywords_.isSet("YRange"))) return Messenger::error("Must supply x and y ranges (e.g. xrange=min,max,delta) when importing 2D cartesian data.\n");

	// Set up our data
	const Vec3<double> xRange = keywords_.asVec3Double("XRange");
	const double xMin = xRange.x;
	const double xMax = xRange.y;
	const double xDelta = xRange.z;
	const Vec3<double> yRange = keywords_.asVec3Double("YRange");
	const double yMin = yRange.x;
	const double yMax = yRange.y;
	const double yDelta = yRange.z;
	data.initialise(xMin, xMax, xDelta, yMin, yMax, yDelta);

	// Loop over lines in the file - we expect blocks with three columns (x, y, f(x,y))
	while (!parser.eofOrBlank())
	{
		// Read line
		if (parser.getArgsDelim() != LineParser::Success) return false;

		// Check number of arguments
		if (parser.nArgs() != 3)
		{
			Messenger::warn("Skipping bad line: %s...\n", parser.line());
			continue;
		}

		double x = parser.argd(0);
		double y = parser.argd(1);
		int xBin = (x - xMin) / xDelta;
		if ((xBin < 0) || (xBin >= data.constXAxis().nItems()))
		{
			Messenger::warn("Coordinates x=%e y=%e are out-of-range (xBin = %i, nBins = %i).\n", x, y, xBin, data.constXAxis().nItems());
			continue;
		}
		int yBin = (y - yMin) / yDelta;
		if ((yBin < 0) || (yBin >= data.constYAxis().nItems()))
		{
			Messenger::warn("Coordinates x=%e y=%e are out-of-range (yBin = %i, nBins = %i).\n", x, y, yBin, data.constYAxis().nItems());
			continue;
		}

		// Set the value
		data.value(xBin, yBin) = parser.argd(2);
	}

	return true;
}
