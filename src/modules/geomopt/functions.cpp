/*
	*** Geometry Optimisation Module - Functions
	*** src/modules/geomopt/functions.cpp
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

#include "modules/geomopt/geomopt.h"
#include "modules/energy/energy.h"
#include "classes/atom.h"
#include "classes/configuration.h"

// Copy coordinates from supplied Configuration into reference arrays
void GeometryOptimisationModule::setReferenceCoordinates(Configuration* cfg)
{
	for (int n=0; n<cfg->nAtoms(); ++n)
	{
		Vec3<double> r = cfg->atom(n)->r();
		xRef_[n] = r.x;
		yRef_[n] = r.y;
		zRef_[n] = r.z;
	}
}

// Revert Configuration to reference coordinates
void GeometryOptimisationModule::revertToReferenceCoordinates(Configuration* cfg)
{
	for (int n=0; n<cfg->nAtoms(); ++n) cfg->atom(n)->setCoordinates(xRef_[n], yRef_[n], zRef_[n]);
}

// Return current RMS force
double GeometryOptimisationModule::rmsForce() const
{
	double rmsf = 0.0;
	for (int n=0; n<xForce_.nItems(); ++n) rmsf += xForce_.constAt(n)*xForce_.constAt(n) + yForce_.constAt(n)*yForce_.constAt(n) + zForce_.constAt(n)*zForce_.constAt(n);
	rmsf /= xForce_.nItems();

	return sqrt(rmsf);
}

// Determine suitable step size from current forces
double GeometryOptimisationModule::gradientStepSize()
{
	double fMax = xForce_.maxAbs();
	double fTemp = yForce_.maxAbs();
	if (fTemp > fMax) fMax = fTemp;
	fTemp = zForce_.maxAbs();
	if (fTemp > fMax) fMax = fTemp;

	return 1.0e-5;
	return 1.0 / fMax;
}

// Sort bounds / energies so that minimum energy is in the central position
void GeometryOptimisationModule::sortBoundsAndEnergies(Vec3<double>& bounds, Vec3<double>& energies)
{
	// Ensure that the energy minimum is the midpoint
	int minVal = energies.minElement();
	if (minVal != 1)
	{
		energies.swap(1, minVal);
		bounds.swap(1, minVal);
	}
}

// Return energy of adjusted coordinates, following the force vectors by the supplied amount
double GeometryOptimisationModule::energyAtGradientPoint(ProcessPool& procPool, Configuration* cfg, const PotentialMap& potentialMap, double delta)
{
	Atom** atoms = cfg->atoms().array();
	for (int n=0; n<cfg->nAtoms(); ++n) atoms[n]->setCoordinates(xRef_[n]+xForce_[n]*delta, yRef_[n]+yForce_[n]*delta, zRef_[n]+zForce_[n]*delta);
	cfg->updateCellContents();

	return EnergyModule::totalEnergy(procPool, cfg, potentialMap);
}

// Perform Golden Search within specified bounds
double GeometryOptimisationModule::goldenSearch(ProcessPool& procPool, Configuration* cfg, const PotentialMap& potentialMap, const double tolerance, Vec3<double>& bounds, Vec3<double>& energies, int& nPointsAccepted)
{
	// Ensure that the energy minimum is the midpoint
	sortBoundsAndEnergies(bounds, energies);

	// Check convergence, ready for early return
	if (fabs(bounds.x-bounds.z) < tolerance) return energies.y;

	// Calculate deltas between bound values
	double dxy = bounds[0] - bounds[1];
	double dyz = bounds[2] - bounds[1];
	Messenger::printVerbose("Trying Golden Search -  %f-%f-%f, dxy = %12.5e, dyz = %12.5e", bounds.x, bounds.y, bounds.z, dxy, dyz);

	// Select largest of two intervals to be the target of the search
	bool xyLargest = fabs(dxy) > fabs(dyz);
	double newMinimum = bounds[1] + 0.3819660 * (xyLargest ? dxy : dyz);

	// Test energy at new trial minimum
	double eNew = energyAtGradientPoint(procPool, cfg, potentialMap, newMinimum);
	Messenger::printVerbose("--> GOLD point is %12.5e [%12.5e] ", eNew, newMinimum);

	// Set order for checking of energy points
	Vec3<int> order(1, xyLargest ? 0 : 2, xyLargest ? 2 : 0);

	// Check each energy to see if our new energy is lower. If it is, overwrite it and recurse
	for (int n=0; n<3; ++n)
	{
		if (eNew < energies[order[n]])
		{
			Messenger::printVerbose("--> GOLD point is lower than point %i...", order[n]);

			// Overwrite the outermost bound with the old minimum
			bounds[xyLargest ? 0 : 2] = newMinimum;
			energies[xyLargest ? 0 : 2] = eNew;

			++nPointsAccepted;

			// Recurse into the new region
			return goldenSearch(procPool, cfg, potentialMap, tolerance, bounds, energies, nPointsAccepted);
		}
	}

	// Nothing better than the current central energy value, so revert to the stored reference coordinates
	revertToReferenceCoordinates(cfg);

	return energies.y;
}

// Line minimise supplied Configuration from the reference coordinates along the stored force vectors
double GeometryOptimisationModule::lineMinimise(ProcessPool& procPool, Configuration* cfg, const PotentialMap& potentialMap, const double tolerance, double& stepSize)
{
	// Brent-style line minimiser with parabolic interpolation and Golden Search backup

	// Set initial bounding values
	Vec3<double> bounds, energies;
	bounds.x = 0.0;
	energies.x = EnergyModule::totalEnergy(procPool, cfg, potentialMap);
	bounds.y = stepSize;
	energies.y = energyAtGradientPoint(procPool, cfg, potentialMap, bounds.y);
	bounds.z = 2.0 * stepSize;
	energies.z = energyAtGradientPoint(procPool, cfg, potentialMap, bounds.z);

	Messenger::printVerbose("Initial bounding values/energies = %12.5e (%12.5e) %12.5e (%12.5e) %12.5e (%12.5e)", bounds[0], energies[0], bounds[1], energies[1], bounds[2], energies[2]);

	// Perform linesearch along the gradient vector
	do
	{
		// Sort w.r.t. energy so that the minimum is in the central point
		sortBoundsAndEnergies(bounds, energies);

		Messenger::printVerbose("Energies [Bounds] = %12.5e (%12.5e) %12.5e (%12.5e) %12.5e (%12.5e)", energies[0], bounds[0], energies[1], bounds[1], energies[2], bounds[2]);

		// Perform parabolic interpolation to find new minimium point
		double b10 = bounds[1] - bounds[0];
		double b12 = bounds[1] - bounds[2];
		double a = (b10 * b10 * (energies[1] - energies[2])) - (b12 * b12 * (energies[1] - energies[0]));
		double b = (b10 * (energies[1] - energies[2])) - (b12 * (energies[1] - energies[0]));
		double newBound = bounds[1] - 0.5 * (a / b);

		// Compute energy of new point and check that it went down...
		double eNew = energyAtGradientPoint(procPool, cfg, potentialMap, newBound);

		Messenger::printVerbose("PARABOLIC point gives energy %12.5e @ %12.5e", eNew, newBound);
		if (eNew < energies[1])
		{
			// New point found...
			Messenger::printVerbose("--> PARABOLIC point is new minimum...");

			// Overwrite the largest of bounds[0] and bounds[2] with the old minimum
			int largest = energies[2] > energies[0] ? 2 : 0;
			bounds.swap(1, largest);
			energies.swap(1, largest);

			// Set the new central values
			bounds[1] = newBound;
			energies[1] = eNew;
		}
		else if ((energies[2] - eNew) > tolerance)
		{
			Messenger::printVerbose("--> PARABOLIC point is better than bounds[2]...");
			bounds[2] = newBound;
			energies[2] = eNew;
		}
		else if ((energies[0] - eNew) > tolerance)
		{
			Messenger::printVerbose("--> PARABOLIC point is better than bounds[0]...");
			bounds[0] = newBound;
			energies[0] = eNew;
		}
		else
		{
			Messenger::printVerbose("--> PARABOLIC point is worse than all current values...");

			// Revert to the stored reference coordinates
			revertToReferenceCoordinates(cfg);

			// Try recursive Golden Search instead, into the largest of the two sections
			int nPointsAccepted = 0;
			goldenSearch(procPool, cfg, potentialMap, tolerance, bounds, energies, nPointsAccepted);
			if (nPointsAccepted == 0) break;
		}
// 		printf("DIFF = %f, 2tol = %f\n", fabs(bounds[0]-bounds[2]), 2.0 * tolerance);
// 		++count;
// 		if (count > 10) break;
	} while (fabs(bounds[0]-bounds[2]) > (2.0 * tolerance));
// 	printf("Final bounding values are %12.5e %12.5e %12.5e\n",bounds[0],bounds[1],bounds[2]);
// 	printf("             energies are %12.5e %12.5e %12.5e\n",energies[0],energies[1],energies[2]);

	// Sort w.r.t. energy so that the minimum is in the central point
	sortBoundsAndEnergies(bounds, energies);

	// Set an updated step size based on the current bounds
	stepSize = (bounds.x + bounds.y + bounds.z);

	return energies.y;
}
