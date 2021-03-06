/*
	*** Neutron SQ Module - Functions
	*** src/modules/neutronsq/functions.cpp
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

#include "modules/neutronsq/neutronsq.h"
#include "classes/configuration.h"
#include "classes/species.h"
#include "classes/speciesinfo.h"
#include "base/lineparser.h"

// Calculate weighted g(r) from supplied unweighted g(r) and Weights
bool NeutronSQModule::calculateWeightedGR(PartialSet& unweightedgr, PartialSet& weightedgr, Weights& weights, NeutronSQModule::NormalisationType normalisation)
{
	int typeI, typeJ;
	for (typeI=0; typeI<unweightedgr.nAtomTypes(); ++typeI)
	{
		for (typeJ=typeI; typeJ<unweightedgr.nAtomTypes(); ++typeJ)
		{
			double weight = weights.weight(typeI, typeJ);
			double boundWeight = weights.boundWeight(typeI, typeJ);

			// Bound (intramolecular) partial (multiplied by the bound term weight)
			weightedgr.boundPartial(typeI, typeJ).copyArrays(unweightedgr.boundPartial(typeI, typeJ));
			weightedgr.boundPartial(typeI, typeJ).values() *= boundWeight;

			// Unbound partial (multiplied by the full weight)
			weightedgr.unboundPartial(typeI, typeJ).copyArrays(unweightedgr.unboundPartial(typeI, typeJ));
			weightedgr.unboundPartial(typeI, typeJ).values() -= 1.0;
			weightedgr.unboundPartial(typeI, typeJ).values() *= weight;

			// Full partial, summing bound and unbound terms
			weightedgr.partial(typeI, typeJ).copyArrays(weightedgr.unboundPartial(typeI, typeJ));
			weightedgr.partial(typeI, typeJ) += weightedgr.boundPartial(typeI, typeJ);
		}
	}

	// Calculate and normalise total to form factor if requested
	weightedgr.formTotal(false);
	if (normalisation == NeutronSQModule::AverageOfSquaresNormalisation) weightedgr.total().values() /= weights.boundCoherentAverageOfSquares();
	else if (normalisation == NeutronSQModule::SquareOfAverageNormalisation) weightedgr.total().values() /= weights.boundCoherentSquareOfAverage();

	return true;
}

// Calculate weighted S(Q) from supplied unweighted S(Q) and Weights
bool NeutronSQModule::calculateWeightedSQ(PartialSet& unweightedsq, PartialSet& weightedsq, Weights& weights, NeutronSQModule::NormalisationType normalisation)
{
	int typeI, typeJ;
	for (typeI=0; typeI<unweightedsq.nAtomTypes(); ++typeI)
	{
		for (typeJ=typeI; typeJ<unweightedsq.nAtomTypes(); ++typeJ)
		{
			// Weight bound and unbound S(Q) and sum into full partial
			double weight = weights.weight(typeI, typeJ);
			double boundWeight = weights.boundWeight(typeI, typeJ);

			// Bound (intramolecular) partial (multiplied by the bound term weight)
			weightedsq.boundPartial(typeI, typeJ).copyArrays(unweightedsq.boundPartial(typeI, typeJ));
			weightedsq.boundPartial(typeI, typeJ).values() *= boundWeight;

			// Unbound partial (multiplied by the full weight)
			weightedsq.unboundPartial(typeI, typeJ).copyArrays(unweightedsq.unboundPartial(typeI, typeJ));
			weightedsq.unboundPartial(typeI, typeJ).values() *= weight;

			// Full partial (sum of bound and unbound terms)
			weightedsq.partial(typeI, typeJ).copyArrays(weightedsq.unboundPartial(typeI, typeJ));
			weightedsq.partial(typeI, typeJ) += weightedsq.boundPartial(typeI, typeJ);
		}
	}

	// Calculate and normalise total to form factor if requested
	weightedsq.formTotal(false);
	if (normalisation == NeutronSQModule::AverageOfSquaresNormalisation) weightedsq.total().values() /= weights.boundCoherentAverageOfSquares();
	else if (normalisation == NeutronSQModule::SquareOfAverageNormalisation) weightedsq.total().values() /= weights.boundCoherentSquareOfAverage();

	return true;
}

// Calculate Weights matrix summed over target Configurations
bool NeutronSQModule::calculateSummedWeights(Weights& summedWeights) const
{
	summedWeights.clear();

	// Loop over Configurations
	RefListIterator<Configuration> configIterator(targetConfigurations_);
	while (Configuration* cfg = configIterator.iterate())
	{
		// Loop over Species used in this Configuration and find its entry in the defined Isotopologues for the Module
		ListIterator<SpeciesInfo> speciesInfoIterator(cfg->usedSpecies());
		while (SpeciesInfo* spInfo = speciesInfoIterator.iterate())
		{
			// Find the Isotopologues for the Configuration/Species, if they have been defined
			const Isotopologues* topes = isotopologues_.isotopologues(cfg, spInfo->species());

			// Use the natural isotopologue if a species in the Configuration is not covered by at least one explicit Isotopologue definition
			if (!topes)
			{
				Messenger::print("Isotopologue specification for Species '%s' in Configuration '%s' is missing, so the natural isotopologue will be used.\n", spInfo->species()->name(), cfg->name());

				Species* sp = spInfo->species();
				summedWeights.addIsotopologue(sp, spInfo->population(), sp->naturalIsotopologue(), 1.0);
			}
			else
			{
				// Add defined isotopologues, in the relative isotopic proportions defined, to the weights.
				ListIterator<IsotopologueWeight> weightIterator(topes->mix());
				while (IsotopologueWeight* isoWeight = weightIterator.iterate()) summedWeights.addIsotopologue(spInfo->species(), spInfo->population(), isoWeight->isotopologue(), isoWeight->weight());
			}
		}
	}

	// Finalise the Weights
	summedWeights.createFromIsotopologues(exchangeableTypes_);

	return true;
}
