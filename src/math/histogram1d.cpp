/*
	*** 1-Dimensional Binned Data With Statistics
	*** src/math/histogram1d.cpp
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

#include "math/histogram1d.h"
#include "base/messenger.h"
#include "base/lineparser.h"

// Static Members (ObjectStore)
template<class Histogram1D> RefList<Histogram1D,int> ObjectStore<Histogram1D>::objects_;
template<class Histogram1D> int ObjectStore<Histogram1D>::objectCount_ = 0;
template<class Histogram1D> int ObjectStore<Histogram1D>::objectType_ = ObjectInfo::Histogram1DObject;
template<class Histogram1D> const char* ObjectStore<Histogram1D>::objectTypeName_ = "Histogram1D";

// Constructor
Histogram1D::Histogram1D() : ListItem<Histogram1D>(), ObjectStore<Histogram1D>(this) 
{
	clear();
}

// Destructor
Histogram1D::~Histogram1D()
{
}

// Copy Constructor
Histogram1D::Histogram1D(const Histogram1D& source) : ObjectStore<Histogram1D>(this)
{
	(*this) = source;
}

// Clear Data
void Histogram1D::clear()
{
	xMin_ = 0.0;
	xMax_ = 0.0;
	binWidth_ = 0.0;
	nBins_ = 0;
	nBinned_ = 0;
	x_.clear();
	y_.clear();
	yAccumulated_.clear();
}

/*
 * Data
 */

// Initialise with specified bin range
void Histogram1D::initialise(double xMin, double xMax, double binWidth)
{
	clear();

	// Store xMin and binWidth
	xMin_ = xMin;
	binWidth_ = binWidth;

	// Clamp xMax_ to nearest bin boundary (not less than the supplied xMax)
	double range = xMax - xMin_;
	nBins_ = range / binWidth_;
	if ((xMin_ + nBins_*binWidth_) < xMax)
	{
		++nBins_;
		xMax_ = xMin_ + nBins_*binWidth_;
	}
	else xMax_ = xMax;

	// Create the arrays
	x_.initialise(nBins_);
	y_.initialise(nBins_);
	yAccumulated_.initialise(nBins_);

	// Create centre-bin array
	double xCentre = xMin + binWidth_*0.5;
	for (int n=0; n<nBins_; ++n, xCentre += binWidth_) x_.add(xCentre);
}

// Reset histogram bins
void Histogram1D::resetBins()
{
	y_ = 0;
	nBinned_ = 0;
}

// Reset all accumulated data
void Histogram1D::resetAll()
{
	y_ = 0;
	yAccumulated_ = 0.0;
	nBinned_ = 0;
}

// Return minimum x value
double Histogram1D::xMin() const
{
	return xMin_;
}

// Return maximum x value
double Histogram1D::xMax() const
{
	return xMax_;
}

// Return bin width
double Histogram1D::binWidth() const
{
	return binWidth_;
}

// Return number of bins
int Histogram1D::nBins() const
{
	return nBins_;
}

// Return Array of x centre-bin values
const Array<double>& Histogram1D::x() const
{
	return x_;
}

/*
 * Operators
 */

// Operator =
void Histogram1D::operator=(const Histogram1D& source)
{
	xMin_ = source.xMin_;
	xMax_ = source.xMax_;
	nBins_ = source.nBins_;
	binWidth_ = source.binWidth_;
	nBinned_ = source.nBinned_;
	x_ = source.x_;
	yAccumulated_ = source.yAccumulated_;
	y_ = source.y_;
}

/*
 * GenericItemBase Implementations
 */

// Return class name
const char* Histogram1D::itemClassName()
{
	return "Histogram1D";
}

// Write data through specified LineParser
bool Histogram1D::write(LineParser& parser)
{
	if (!parser.writeLineF("%s\n", objectName())) return false;
	if (!parser.writeLineF("%f %f %f\n", xMin_, xMax_, binWidth_)) return false;
	if (!parser.writeLineF("%i\n", nBinned_)) return false;
	for (int n=0; n<nBins_; ++n) if (!yAccumulated_.at(n).write(parser)) return false;
	return true;
}

// Read data through specified LineParser
bool Histogram1D::read(LineParser& parser)
{
	clear();

	if (parser.readNextLine(LineParser::Defaults) != LineParser::Success) return false;
	setObjectName(parser.line());

	if (parser.getArgsDelim(LineParser::Defaults) != LineParser::Success) return false;
	initialise(parser.argd(0), parser.argd(1), parser.argd(2));

	if (parser.getArgsDelim(LineParser::Defaults) != LineParser::Success) return false;
	nBinned_ = parser.argi(0);

	for (int n=0; n<nBins_; ++n) if (!yAccumulated_.at(n).read(parser)) return false;

	return true;
}

/*
 * Parallel Comms
 */

// Broadcast data
bool Histogram1D::broadcast(ProcessPool& procPool, int rootRank)
{
#ifdef PARALLEL
	// Range data
	if (!procPool.broadcast(xMin_, rootRank)) return false;
	if (!procPool.broadcast(xMax_, rootRank)) return false;
	if (!procPool.broadcast(binWidth_, rootRank)) return false;
	if (!procPool.broadcast(nBins_, rootRank)) return false;

	// Data
	if (!procPool.broadcast(nBinned_, rootRank)) return false;
	if (!procPool.broadcast(x_, rootRank)) return false;
	if (!procPool.broadcast(y_, rootRank)) return false;
	if (!procPool.broadcast(yAccumulated_, rootRank)) return false;
#endif
	return true;
}

// Check item equality
bool Histogram1D::equality(ProcessPool& procPool)
{
#ifdef PARALLEL
	// Check number of items in arrays first
	if (!procPool.equality(nPoints())) return Messenger::error("Histogram1D number of points is not equivalent (process %i has %i).\n", procPool.poolRank(), nPoints());
	for (int n=0; n<nPoints(); ++n)
	{
		if (!procPool.equality(x_[n])) return Messenger::error("Histogram1D x value %i is not equivalent (process %i has %e).\n", n, procPool.poolRank(), x_[n]);
		if (!procPool.equality(y_[n])) return Messenger::error("Histogram1D y value %i is not equivalent (process %i has %e).\n", n, procPool.poolRank(), y_[n]);
	}
	if (!procPool.equality(z_)) return Messenger::error("Histogram1D z value is not equivalent (process %i has %e).\n", procPool.poolRank(), z_);
#endif
	return true;
}