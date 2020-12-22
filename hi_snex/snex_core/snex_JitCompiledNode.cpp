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


namespace snex {
namespace jit {
using namespace juce;

int JitCompiledNode::getNumRequiredDataObjects(ExternalData::DataType t) const
{
	return numRequiredDataTypes[(int)t];
}

void JitCompiledNode::setExternalData(int index, const ExternalData& b)
{
	setExternalDataFunction.callVoid(index, &b);
}

JitCompiledNode::JitCompiledNode(Compiler& c, const String& code, const String& classId, int numChannels_, const CompilerInitFunction& cf) :
	r(Result::ok()),
	numChannels(numChannels_)
{
	String s;

	memset(numRequiredDataTypes, 0, sizeof(int) * (int)ExternalData::DataType::numDataTypes);

	auto implId = NamespacedIdentifier::fromString("impl::" + classId);

	s << "namespace impl { " << code;
	s << "}\n";
	s << implId.toString() << " instance;\n";

	cf(c, numChannels);

	Array<Identifier> fIds;

	for (auto f : Types::ScriptnodeCallbacks::getAllPrototypes(c, numChannels))
	{
		addCallbackWrapper(s, f);
		fIds.add(f.id.getIdentifier());
	}

	// Cheapmaster solution 2000
	if (s.contains("void setExternalData(int"))
	{
		s << "void setExternalData(int v, const ExternalData& b) { instance.setExternalData(v, b); }\n";
	}

	obj = c.compileJitObject(s);

	DBG(c.getAssemblyCode());

	r = c.getCompileResult();

	if (r.wasOk())
	{
		NamespacedIdentifier impl("impl");

		if (instanceType = c.getComplexType(implId))
		{
			if (auto libraryNode = dynamic_cast<SnexNodeBase*>(instanceType.get()))
			{
				parameterList = libraryNode->getParameterList();
			}
			if (auto st = dynamic_cast<StructType*>(instanceType.get()))
			{
				auto pId = st->id.getChildId("Parameters");
				auto pNames = c.getNamespaceHandler().getEnumValues(pId);

				if (!pNames.isEmpty())
				{
					for (int i = 0; i < pNames.size(); i++)
						addParameterMethod(s, pNames[i], i);
				}

				cf(c, numChannels);

				obj = c.compileJitObject(s);
				r = c.getCompileResult();

				auto& nh = c.getNamespaceHandler();

				numRequiredDataTypes[(int)ExternalData::DataType::Table] = nh.getConstantValue(st->id.getChildId("NumTables")).toInt();
				numRequiredDataTypes[(int)ExternalData::DataType::SliderPack] = nh.getConstantValue(st->id.getChildId("NumSliderPacks")).toInt();
				numRequiredDataTypes[(int)ExternalData::DataType::AudioFile] = nh.getConstantValue(st->id.getChildId("NumAudioFiles")).toInt();

				instanceType = c.getComplexType(implId);

				for (auto& n : pNames)
				{
					auto f = obj[Identifier("set" + n)];

					OpaqueSnexParameter osp;
					osp.name = n;
					osp.function = f.function;
					parameterList.add(osp);
				}
			}

			FunctionClass::Ptr fc = instanceType->getFunctionClass();

			thisPtr = obj.getMainObjectPtr();
			ok = true;



			for (int i = 0; i < fIds.size(); i++)
			{
				callbacks[i] = obj[fIds[i]];

				Array<FunctionData> matches;

				fc->addMatchingFunctions(matches, fc->getClassName().getChildId(fIds[i]));

				FunctionData wrappedFunction;

				if (matches.size() == 1)
					wrappedFunction = matches.getFirst();
				else
				{
					for (auto m : matches)
					{
						if (m.matchesArgumentTypes(callbacks[i]))
						{
							wrappedFunction = m;
							break;
						}
					}
				}

				if (!wrappedFunction.matchesArgumentTypes(callbacks[i]))
				{
					r = Result::fail(wrappedFunction.getSignature({}, false) + " doesn't match " + callbacks[i].getSignature({}, false));
					ok = false;
					break;
				}

				if (callbacks[i].function == nullptr)
					ok = false;
			}

			setExternalDataFunction = obj["setExternalData"];


		}
		else
		{
			jassertfalse;
		}
	}
}


}
}