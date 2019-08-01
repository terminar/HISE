/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;

template <typename R> class FunctionParser : public FunctionParserBase
{
public:
	
	FunctionParser(JITScope::Pimpl* scope_, const FunctionParserInfo& info_):
		FunctionParserBase(scope_, info_) {}

	~FunctionParser();

	void addVoidReturnStatement() override;
	void storeGlobalsBeforeReturn();

	void parseReturn() override;

	BaseNodePtr parseParameterReferenceTyped(const Identifier& id) override;

#if INCLUDE_BUFFERS
	void parseBufferLine(const Identifier &id) override
	{
		parseIdentifier();

		if (matchIf(JitTokens::openBracket))
		{
			parseBufferAssignment(id);
			
		}

		match(JitTokens::semicolon);
	}
#endif
	
};




} // end namespace jit
} // end names