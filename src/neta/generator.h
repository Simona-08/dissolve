/*
	*** NETADefinition Generator
	*** src/neta/generator.h
	Copyright T. Youngs 2015-2020

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

#ifndef DISSOLVE_NETA_GENERATOR_H
#define DISSOLVE_NETA_GENERATOR_H

#include "base/charstring.h"
#include "base/enumoptions.h"
#include "templates/pointerarray.h"
#include "templates/reflist.h"

// External declarations
extern int NETADefinitionGenerator_parse();

// Forward declarations
class Element;
class Forcefield;
class ForcefieldAtomType;
class NETAConnectionNode;
class NETADefinition;
class NETANode;

// NETADefinition Generator
class NETADefinitionGenerator
{
	private:
	// Constructor
	NETADefinitionGenerator(NETADefinition& definition, const char* definitionText, const Forcefield* associatedFF = NULL);

	public:
	// Destructor
	~NETADefinitionGenerator();


	/*
	 * Target Objects
	 */
	private:
	// Pointer to target NETADefinition
	static NETADefinition* definition_;
	// The NETADefinitionGenerator object calling the parser/lexer
	static NETADefinitionGenerator* generator_;

	public:
	// Return target NETADefinition (static to allow NETADefinitionGenerator_parse() to use it)
	static NETADefinition* definition();
	// Return current NETADefinitionGenerator (static to allow NETADefinitionGenerator_parse() to use it)
	static NETADefinitionGenerator* generator();


	/*
	 * Lexer
	 */
	private:
	// Source definition string
	CharString definitionString_;
	// Integer position in stringSource, total length of string, and starting position of current token/function
	int stringPos_, stringLength_, tokenStart_, functionStart_;

	private:
	// Set string source for lexer
	void setSource(const char* definitionText);
	// Get next character from current input stream
	char getChar();
	// Peek next character from current input stream
	char peekChar();
	// 'Replace' last character read from current input stream
	void unGetChar();

	public:
	// Parser lexer, called by yylex()
	int lex();


	/*
	 * Creation
	 */
	private:
	// Encompassing forcefield for the definition (if any)
	static const Forcefield* associatedForcefield_;
	// Temporary element array used in definition creation
	static PointerArray<Element> targetElements_;
	// Temporary atomtype array used in definition creation
	static PointerArray<ForcefieldAtomType> targetAtomTypes_;
	// Context (branch) stack
	static RefList<NETANode> contextStack_;
	// Whether to recognise text elements as generic names, rather than an element or unrecognised token
	static bool expectName_;

	public:
	// Add element target to array (by Z)
	static bool addElementTarget(int elementZ);
	// Add atomtype target to array (by id)
	static bool addAtomTypeTarget(int id);
	// Add atomtype target to array (by name)
	static bool addAtomTypeTarget(const char* typeName);
	// Return target Elements array
	static PointerArray<Element> targetElements();
	// Return target ForcefieldAtomTypes array
	static PointerArray<ForcefieldAtomType> targetAtomTypes();
	// Clear element / atomtype targets
	static void clearTargets();
	// Return topmost context
	static NETANode* context();
	// Push branch in last node of topmost context onto the context stack
	static bool pushContext();
	// Pop topmost context
	static void popContext();
	// Set whether to recognise text elements as generic names
	static void setExpectName(bool b);
	// Static generation function
	static bool generate(NETADefinition& neta, const char* netaDefinition, const Forcefield* associatedFF);
};

#endif
