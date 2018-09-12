/*
	*** 3-Dimensional Data
	*** src/math/data3d.h
	Copyright T. Youngs 2012-2018

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

#ifndef DISSOLVE_DATA3D_H
#define DISSOLVE_DATA3D_H

#include "math/plottable.h"
#include "templates/array3d.h"
#include "templates/objectstore.h"

// Forward Declarations
class Histogram3D;

// One-Dimensional Data
class Data3D : public Plottable, public ListItem<Data3D>, public ObjectStore<Data3D>, public GenericItemBase
{
	public:
	// Constructor
	Data3D();
	// Destructor
	~Data3D();
	// Copy Constructor
	Data3D(const Data3D& source);
	// Clear data
	void clear();
	

	/*
	 * Data
	 */
	private:
	// X axis array
	Array<double> x_;
	// Y axis array
	Array<double> y_;
	// Z axis array
	Array<double> z_;
	// Values at each xyz
	Array3D<double> values_;
	// Whether data has associated errors
	bool hasError_;
	// Errors of values, if present
	Array3D<double> errors_;

	public:
	// Initialise arrays to specified size
	void initialise(int xSize, int ySize, int zSize, bool withError = false);
	// Initialise to be consistent in size and axes with supplied object
	void initialise(const Data3D& source);
	// Copy arrays from supplied object
	void copyArrays(const Data3D& source);
	// Zero values array
	void zero();
	// Accumulate specified histogram data
	void accumulate(const Histogram3D& source);
	// Return x axis value specified
	double& xAxis(int index);
	// Return x axis value specified (const)
	double constXAxis(int index) const;
	// Return x axis Array
	Array<double>& xAxis();
	// Return x axis Array (const)
	const Array<double>& constXAxis() const;
	// Return y axis value specified
	double& yAxis(int index);
	// Return y axis value specified (const)
	double constYAxis(int index) const;
	// Return y axis Array
	Array<double>& yAxis();
	// Return y axis Array (const)
	const Array<double>& constYAxis() const;
	// Return z axis value specified
	double& zAxis(int index);
	// Return z axis value specified (const)
	double constZAxis(int index) const;
	// Return z axis Array
	Array<double>& zAxis();
	// Return z axis Array (const)
	const Array<double>& constZAxis() const;
	// Return value specified
	double& value(int xIndex, int yIndex, int zIndex);
	// Return value value specified (const)
	double constValue(int xIndex, int yIndex, int zIndex) const;
	// Return value Array
	Array3D<double>& values();
	// Return values Array
	const Array3D<double>& constValues3D() const;
	// Add / initialise errors array
	void addErrors();
	// Return error value specified
	double& error(int xIndex, int yIndex, int zIndex);
	// Return error value specified (const)
	double constError(int xIndex, int yIndex, int zIndex) const;
	// Return error Array
	Array3D<double>& errors();
	// Return errors Array
	const Array3D<double>& constErrors3D() const;


	/*
	 * Operators
	 */
	public:
	// Assignment Operator
	void operator=(const Data3D& source);
	// Operator +=
	void operator+=(const Data3D& source);
	// Operator +=
	void operator+=(const double delta);
	// Operator -=
	void operator-=(const Data3D& source);
	// Operator -=
	void operator-=(const double delta);
	// Operator *=
	void operator*=(const double factor);
	// Operator /=
	void operator/=(const double factor);


	/*
	 * File I/O
	 */
	public:
	// Load data from specified LineParser, using columns specified
	bool load(LineParser& parser, int xcol = 0, int ycol = 1);
	// Load data from specified file, using columns specified
	bool load(const char* filename, int xcol = 0, int ycol = 1);
	// Load data from specified file through ProcessPool, using columns specified
	bool load(ProcessPool& pool, const char* filename, int xcol = 0, int ycol = 1);
	// Save data to specified filevalues
	bool save(const char* filename) const;


	/*
	 * Plottable Implementation
	 */
	public:
	// Return number of points along x axis
	int nXAxisPoints() const;
	// Return minimum (first) x axis point
	double xAxisMin() const;
	// Return maximum (last) x axis point
	double xAxisMax() const;
	// Return number of points along y axis
	int nYAxisPoints() const;
	// Return minimum (first) y axis point
	double yAxisMin() const;
	// Return maximum (last) y axis point
	double yAxisMax() const;
	// Return number of points along z axis
	int nZAxisPoints() const;
	// Return minimum (first) z axis point
	double zAxisMin() const;
	// Return maximum (last) z axis point
	double zAxisMax() const;
	// Return number of datapoints present in whole dataset
	int nDataPoints() const;
	// Return minimum value over all data points
	double minValue() const;
	// Return maximum value over all data points
	double maxValue() const;
	// Return whether the values have associated errors
	bool valuesHaveErrors() const;


	/*
	 * GenericItemBase Implementations
	 */
	public:
	// Return class name
	static const char* itemClassName();
	// Write data through specified LineParser
	bool write(LineParser& parser);
	// Read data through specified LineParser
	bool read(LineParser& parser);


	/*
	 * Parallel Comms
	 */
	public:
	// Broadcast data
	bool broadcast(ProcessPool& procPool, int rootRank = 0);
	// Check item equality
	bool equality(ProcessPool& procPool);
};

#endif