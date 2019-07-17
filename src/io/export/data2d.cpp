/*
	*** Export - Data2D
	*** src/io/export/data2d.cpp
	Copyright T. Youngs 2012-2019

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

#include "io/export/data2d.h"
#include "math/data2d.h"
#include "base/lineparser.h"
#include "base/sysfunc.h"

// Data2D Export Keywords
const char* Data2DExportFormatKeywords[] = { "block" };
const char* NiceData2DExportFormatKeywords[] = { "Block Data" };

// Return number of available formats
int Data2DExportFileFormat::nFormats() const
{
	return Data2DExportFileFormat::nData2DExportFormats;
}

// Return formats array
const char** Data2DExportFileFormat::formats() const
{
	return Data2DExportFormatKeywords;
}

// Return nice formats array
const char** Data2DExportFileFormat::niceFormats() const
{
	return NiceData2DExportFormatKeywords;
}

// Return current format as CoordinateExportFormat
Data2DExportFileFormat::Data2DExportFormat Data2DExportFileFormat::data2DFormat() const
{
	return (Data2DExportFileFormat::Data2DExportFormat) format_;
}

// Constructor
Data2DExportFileFormat::Data2DExportFileFormat(const char* filename, Data2DExportFormat format) : FileAndFormat(filename, format)
{
}

/*
 * Export Functions
 */

// Export Data2D as simple block data
bool Data2DExportFileFormat::exportBlockData(LineParser& parser, Data2D& data)
{
	// Export header comment
	if (!parser.writeLineF("# %i blocks (nX) of %i points (nY).\n", data.xAxis().nItems(), data.yAxis().nItems())) return false;

	// Export datapoints, separating each block of a specific x value with a single blank line
	const Array2D<double>& values = data.constValues2D();
	for (int x=0; x<values.nRows(); ++x)
	{
		for (int y=0; y<values.nColumns(); ++y) if (!parser.writeLineF("%15.9f\n", values.constAt(x,y))) return false;
		if (!parser.writeLineF("\n")) return false;
	}

	return true;
}

// Export Data2D using current filename and format
bool Data2DExportFileFormat::exportData(Data2D& data)
{
	// Open the file
	LineParser parser;
	if (!parser.openOutput(filename_))
	{
		parser.closeFiles();
		return false;
	}

	// Write data
	bool result = false;
	if (data2DFormat() == Data2DExportFileFormat::BlockData) result = exportBlockData(parser, data);
	else Messenger::error("Unrecognised data format.\nKnown formats are: %s.\n", Data2DExportFileFormat().formats());

	return false;
}