
/* Copyright (c) 2007-2021 SynthEdit Ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name SEM, nor SynthEdit, nor 'Music Plugin Interface' nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY SynthEdit Ltd ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SynthEdit Ltd BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mp_sdk_gui2.h"
#include "Drawing.h"
#include "jsoncpp/json/json.h"
#include "Cadmium/GUI_3_0.h"
#include "../se_sdk3/TimerManager.h"

using namespace gmpi;
using namespace GmpiDrawing;

struct moduleConnection
{
	int32_t toModuleHandle = {};
	int32_t fromModulePin = {};
	int32_t toModulePin = {};
};

struct moduleInfo
{
	int32_t sort = -1; // unsorted
//	int32_t moduleHandle = {};
	std::vector<moduleConnection> connections;
};

void CalcSortOrder2(std::map<int32_t, moduleInfo>& allModules, moduleInfo& m, int maxSortOrderGlobal)
{
	m.sort = -2; // prevent recursion back to this.
	int maxSort = -1; // reset.
	for (auto connector : m.connections)
	{
		assert(allModules.find(connector.toModuleHandle) != allModules.end());
		auto& to = allModules[connector.toModuleHandle];
		{
			{
				int order = to.sort;
				if (order == -2) // Found an feedback path, report it.
				{
					assert(false); // FEEDBACK!
					return;
					/*
											m.sort = -1; // Allow this to be re-sorted after feedback (potentially) compensated.
											auto e = new FeedbackTrace(SE_FEEDBACK_PATH);
											e->AddLine(p, to);
											return e;
					*/
				}

				if (order == -1) // Found an unsorted path, go down it.
				{
					/*auto e = */ CalcSortOrder2(allModules, to, maxSortOrderGlobal);
#if 0
					if (e) // Downstream module encountered feedback.
					{
						// Not all modules have valid moduleType, e.g. oversampler_in
						if (moduleType && (moduleType->GetFlags() & CF_IS_FEEDBACK) != 0 && p->DataType == DT_MIDI) // dummy pin
						{
							// User has inserted a feedback module, activate it by removing dummy connection.
							auto dummy = plugs.back();
							dummy->connections.front()->connections.clear();
							dummy->connections.clear();

							// Feedback fixed, remove feedback trace.
							delete e;
							e = nullptr;

							// Continue as if nothing happened.
							goto done;
						}
						else
						{
							SetSortOrder(-1); // Allow this to be re-sorted after feedback (potentially) compensated.

							// If downstream module has feedback, add trace information.
							e->AddLine(p, to);
							if (e->feedbackConnectors.front().second->UG == this) // only reconstruct feedback loop as far as nesc.
							{
#if defined( SE_SUPPORT_MFC )
								throw e;
#else
#if defined( _DEBUG )
								e->DebugDump();
#endif
#endif
							}
							return e;
						}
					}
					else
#endif
					{
						order = to.sort;// ->UG->GetSortOrder(); // now sorted. take into account.
					}
				}

				maxSort = (std::max)(maxSort, order);
			}
		}
	}

	// We've searched all downstream modules. none are unsorted.
	// Set my sort to my max downstream sort + 1.
//	if (false) // Bredth-first sort.
	/* buggy in some situations, put MIDI-CV AFTER Voice-Mute module.
	if (maxSort == -1 || joinedExistingSortedPath) // Depth-first sort. About 1.5% faster.
	{
		assert(maxSortOrderGlobal >= maxSort);
		maxSort = maxSortOrderGlobal;
	}
	*/

//done:
	++maxSort;

	assert(maxSort > -1);

	/* should be fixed via UGF_UPSTREAM _PARAMETER mechanism
	// a drum voice can easily have modules 'upstream' of 'drum_trigger' module,
	// however, on Resume(), those modules skip one audio block, causing glitch.
	// to prevent this, set notesource's sort order much higher than it's predecessors.
	// this is not an absolute guarantee fix, but should be ok 99% of the time
	if((GetFlags() & UGF_UPSTREAM _PATCH_MGR) != 0)
	{
		maxSort += 200;
	}
	*/

	// SetSortOrder(maxSort);
	m.sort = maxSort;
	maxSortOrderGlobal = (std::max)(maxSort, maxSortOrderGlobal);
}

// ref ug_container::SortOrderSetup()
void SortModules(std::map<int32_t, moduleInfo>& allModules)
{
	int maxSortOrderGlobal = -1;

	for (auto& it : allModules)
	{
		auto& m = it.second;

		if (m.sort >= 0) // Skip sorted modules.
		{
			continue;
		}

		CalcSortOrder2(allModules, m, maxSortOrderGlobal);
	}
}

class CadmiumRenderer final : public gmpi_gui::MpGuiGfxBase, public TimerClient
{
	StringGuiPin pinJson;

	functionalUI functionalUI;
	std::vector<functionalUI::node*> renderNodes2;

	void update()
	{
		//TODO?		functionalUI.states2.clear();

		//TODO just make 'states2' and 'nodes' a member of me? no real need for seperate 'functionalUI' object.

		//// by convention, first state is the render context.
		//const auto rendercontextIdx = functionalUI.states2.size();
		//functionalUI.states2.push_back(
		//	std::make_unique<functionalUI::state_t>(vGraphicsContext())
		//);

		// de-serialize JSON into object graph.
		Json::Value document_json;// = new Json::Value();
		{
			Json::Reader reader;
			const auto jsonString = (std::string)pinJson;
			reader.parse(jsonString, document_json);
		}

		// sort modules (actually a 'dumb' representiative of the module)
		std::map<int32_t, moduleInfo> moduleSort;
		for (auto& module_json : document_json["connections"])
		{
			const auto fromHandle = module_json["fMod"].asInt();
			const auto toHandle = module_json["tMod"].asInt();
			const auto fromPin = module_json["fPin"].asInt(); // not needed while nodes have only one output? (always output node).
			const auto toPin = module_json["tPin"].asInt();

			moduleSort[fromHandle].connections.push_back(
				{
					toHandle,
					fromPin,
					toPin
				}
			);

			auto unused = moduleSort[toHandle].sort; // ensure we have a record for all 'to' modules.
		}

		SortModules(moduleSort);

#if 0 //def _DEBUG
		// print out sort result
		for (auto& it : moduleSort)
		{
			auto& m = it.second;
			_RPTN(0, "M %d : sort %d\n", it.first, m.sort);
		}
#endif

		std::vector<int32_t> renderNodeHandles;

		for (auto& module_json : document_json["modules"])
		{
			const auto typeName = module_json["type"].asString();
			const auto handle = module_json["handle"].asInt();

			auto it = factory.find(typeName);
			if (it != factory.end())
			{
				auto& createNode = it->second;

				createNode(handle, module_json, functionalUI.states2, functionalUI.graph);
			}

			if ("SE Render" == typeName) // should be called fill-geometry or suchlike
			{
				// note down the render nodes, because we need to provide them with the graphics context later.
				renderNodeHandles.push_back(handle);
			}
		}

		// sort actual nodes in line with 'moduleSort' structure.
		std::sort(
			functionalUI.graph.begin(),
			functionalUI.graph.end(),
			[&moduleSort](const functionalUI::node& n1, const functionalUI::node& n2)
				{
					return moduleSort[n1.handle].sort > moduleSort[n2.handle].sort;
				}
			);
		

		std::unordered_map<int32_t, size_t> handleToIndex;
		for (int index = 0; index < functionalUI.graph.size(); ++index)
		{
			handleToIndex.insert({ functionalUI.graph[index].handle, index });
		}

		for (auto handle : renderNodeHandles)
		{
			const auto index = handleToIndex[handle];
			renderNodes2.push_back(&functionalUI.graph[index]);
		}

		/* might be needed, probably not
				// connections (geom, renderer, brush)
		if (functionalUI.renderNodes.empty())
			return;
*/
		for (auto& module_json : document_json["connections"])
		{
			const auto fromHandle = module_json["fMod"].asInt();
			const auto toHandle = module_json["tMod"].asInt();
			const auto fromPin = module_json["fPin"].asInt(); // not needed while nodes have only one output? (always output node).
			const auto toPin = module_json["tPin"].asInt();

			std::vector<functionalUI::state_t*>* destArguments = {};
			if (auto it = handleToIndex.find(toHandle); it != handleToIndex.end())	// must be render target.
			{
				const auto toNodeIndex = (*it).second;
				destArguments = &functionalUI.graph[toNodeIndex].arguments;
				/* might be needed, probably not
							else
				{
					destArguments = &functionalUI.renderNodes[0].arguments;
				}
				*/
				// pad any inputs that have not been connected yet.
				while (destArguments->size() <= toPin)
				{
					destArguments->push_back({});
				}

				const auto fromNodeIndex = handleToIndex[fromHandle];
				(*destArguments)[toPin] = &functionalUI.graph[fromNodeIndex].result;
			}
		}

		// lastly hook rendernodes up to special graphics context state object.
		//for (auto node : renderNodes2)
		//{
		//	node->arguments.push_back(functionalUI.states2[rendercontextIdx].get());
		//}

		functionalUI.step();
//		StartTimer();
	}

	bool OnTimer() override
	{
		functionalUI.step();
		invalidateRect(); // TODO: only if anything changed

		return true;
	}

	std::unordered_map<std::string, std::function<void(int32_t, Json::Value&, std::vector< std::unique_ptr<functionalUI::state_t> >&, std::vector<functionalUI::node>&)> > factory;

public:
	CadmiumRenderer()
	{
		initializePin(pinJson, static_cast<MpGuiBaseMemberPtr2>(&CadmiumRenderer::update) );


		// Init factory
		factory.insert
		(
			{ "SE Solid Color Brush",
			[](int32_t handle, Json::Value& module_json, std::vector< std::unique_ptr<functionalUI::state_t> >& states, std::vector<functionalUI::node>& graph)
			{
				Color brushColor = Color::Black;

				ScanPinDefaults(module_json,
					[&brushColor](int idx, const std::string& value)
				{
					if (idx == 0) // square has only one dimension at present
					{
						brushColor = Color::FromHexStringU(value);
					}
				}
				);

				// Add state for input value (color)
				const auto input1Idx = states.size();
				states.push_back(
					std::make_unique<functionalUI::state_t>(brushColor)
				);

				// Add function
				graph.push_back(
					{
						[](std::vector<functionalUI::state_t*> statesx) -> functionalUI::state_t
						{
							return vBrush(std::get<Color>(*statesx[0]));
						},
						{states.back().get()},
						0.0f,
						handle
					}
				);
			}
			});

		//////////////////////////////////////////////////////////////////
		factory.insert
		(
			{ "CD Circle",
		[](int32_t handle, Json::Value& module_json, std::vector< std::unique_ptr<functionalUI::state_t> >& states, std::vector<functionalUI::node>& graph)
		{
			// !!! actually a circle at present
			float radius{ 10.f };
			Point center{ 100.0f, 100.0f };

			ScanPinDefaults(module_json,
				[&radius](int idx, const std::string& value)
				{
					if (idx == 0) // square has only one dimension at present
					{
						radius = std::stof(value);
					}
				}
			);

			// TODO!!! this needs to come from pin default.
			// center
			const auto input1Idx = states.size();
			states.push_back(
				std::make_unique<functionalUI::state_t>(center)
			);

			// radius
			const auto input2Idx = states.size();
			states.push_back(
				std::make_unique<functionalUI::state_t>(radius)
			);

			// 2. circle geometry
			graph.push_back(
				{
				[](std::vector<functionalUI::state_t*> states) -> functionalUI::state_t
					{
						const auto& center = std::get<Point>(*states[0]);
						const auto& radius = std::get<float>(*states[1]);

						return vCircleGeometry(center, radius);
					},
					{
						states[input1Idx].get(),
						states[input2Idx].get()
					},
					0.0f,
					handle
				}
			);
		}
			}
		);
		//////////////////////////////////////////////////////////////////
		factory.insert
		(
			{ "CD Square",
		[](int32_t handle, Json::Value& module_json, std::vector< std::unique_ptr<functionalUI::state_t> >& states, std::vector<functionalUI::node>& graph)
		{
			float size{ 10.f };
			Point center{ 100.0f, 100.0f };

			ScanPinDefaults(module_json,
				[&size](int idx, const std::string& value)
				{
					if (idx == 0) // square has only one dimension at present
					{
						size = std::stof(value);
					}
				}
			);

			// TODO!!! this needs to come from pin default.
			// center
			const auto input1Idx = states.size();
			states.push_back(
				std::make_unique<functionalUI::state_t>(center)
			);

			const auto input2Idx = states.size();
			states.push_back(
				std::make_unique<functionalUI::state_t>(size)
			);

			graph.push_back(
				{
				[](std::vector<functionalUI::state_t*> states) -> functionalUI::state_t
					{
						const auto& center = std::get<Point>(*states[0]);
						const auto& size = std::get<float>(*states[1]);

						return vSquareGeometry(center, size);
					},
					{
						states[input1Idx].get(),
						states[input2Idx].get()
					},
					0.0f,
					handle
				}
			);
		}
			}
		);

		//////////////////////////////////////////////////////////////////
		factory.insert
		(
			{ "SE Render",
		[](int32_t handle, Json::Value& module_json, std::vector< std::unique_ptr<functionalUI::state_t> >& states, std::vector<functionalUI::node>& graph)
		{
			// 0. render
			graph.push_back(
				{
					[](std::vector<functionalUI::state_t*> states) -> functionalUI::state_t
					{
						auto& brush = std::get<vBrush>(*states[0]);

						vGeometry* geometry = {};
						if(states.size() > 1)
						{
							geometry = std::get_if<vCircleGeometry>(states[1]);
							if (!geometry)
							{
								geometry = std::get_if<vSquareGeometry>(states[1]);
							}
						}

						// this technique pre-calculates the varient 'get's just once. could apply this to ALL nodes? for extra efficiency.
						return RendererX(
						[&brush, geometry](Graphics& g) -> void
						{
							if (geometry) // try to screen the need for this out
							{
								g.FillGeometry(geometry->native(g), brush.native(g));
							}
						});
					},
					{},
					0.0f,
					handle
				}
			);

			}
			}
		);
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		Graphics g(drawingContext);
#if 1
		// store the graphics context in the first state.
//		const int rendercontextIdx = 0;
//		auto& graphicsContextState = std::get<vGraphicsContext>(*functionalUI.states2[rendercontextIdx]);
//		graphicsContextState.native = &g;


		// TODO, only process nodes with 'dirty' input states.
		// SPECIAL CASE: render node should have graphics context marked 'dirty' every time.
		// each state could have a pointer/index to the first node that depends on it. so for example if
		// only graphics context is dirty, we could jump to the final node in the list, very efficient.
		// 'constant' nodes that are init only at start, should be moved to highest sort-order. not gaurenteed at present.
//		functionalUI.draw(g);
		for (auto& rendernode : renderNodes2)
		{
			// the render node produces a higher-order function that is responsible for the actual rendering.
			// TODO if(n->dirty){n->dirty=false;
			std::get<RendererX>(rendernode->result).function(g);
		}
#else
		const auto boundsRect = getRect();

		// TODO pushTransformRAii()
		const auto originalTransform = g.GetTransform();
		const auto adjustedTransform = Matrix3x2::Translation(boundsRect.getWidth() * 0.5f, boundsRect.getHeight() * 0.5f) * originalTransform;
		g.SetTransform(adjustedTransform);

		auto textFormat = GetGraphicsFactory().CreateTextFormat();
		auto brush = g.CreateSolidColorBrush(Color::FromRgb(0xed872d)); // Color::Orange);

//		g.DrawTextU("Hello World!", textFormat, 0.0f, 0.0f, brush);

		Rect rect(-50, -50, 50, 50);
		g.FillRectangle(rect, brush);

		g.SetTransform(originalTransform);
#endif
		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = Register<CadmiumRenderer>::withId(L"SE CadmiumRenderer");
}
