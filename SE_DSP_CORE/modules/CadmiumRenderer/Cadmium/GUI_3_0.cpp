#include "Drawing.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <variant>
#include <functional>
#include "GUI_3_0.h"
#include "IGuiHost2.h"
#include "mfc_emulation.h"

using namespace GmpiDrawing;

list_function::list_function(std::function<state_t(const List&)> pfunction)
 : function(pfunction)
{
}

bool list_function::operator==(const list_function& other)
{
    // TODO: how to compare functions?
    return false;
}

state_t apply_list_function(list_function f, const List& l)
{
    return f.function(l);
}


bool compareState(const state_data_t& lhs, const state_data_t& rhs)
{
//#ifdef _WIN32 // TODO, can't compare std::function for equality on macos. i.e. 'list_function'
	if (lhs.index() == 0 || rhs.index() == 0)
        return false;
        
    return lhs == rhs;
//#endif
//    test_t a, b;
//    return a == b;
    
	return false;
	// comparing two lists of pointers to the same states will always result in false,
	// becuase you're not comparing what the states *were* with what they are now,
	// you're merely comparing them to themselves *now*.

/*
	// Dealing with lists.
	const auto& list1 = std::get<0>(lhs);
	const auto& list2 = std::get<0>(rhs);

	if (list1.size() != list2.size())
		return false;

	for (int i = 0; i < list1.size(); ++i)
	{
		if (!compareState(list1[i]->value, list2[i]->value))
			return false;
	}
	return true;
*/
}


observableState& observableState::operator=(const state_data_t& newvalue)
{
    if (!compareState(value, newvalue))
    {
        value = newvalue;
        for (auto& downstream : downstreamNodes)
        {
            downstream->dirty = true;
        }
    }
    return *this;
}

void node::operator()()
{
	auto newvalue = function(arguments);

	_RPTN(0, "NODE:%s => %s\n", debug_name.c_str(), typeNames[newvalue.index()]);

	if (!compareState(result.value, newvalue))
	{
		result.value = newvalue;
		for (auto& downstream : result.downstreamNodes)
		{
			downstream->dirty = true;
		}
	}
}


// update an input state (from a parameter) such that it will not take affect until the *next* frame.
void functionalUI::updateState(observableState& state, state_data_t newValue)
{
	updates.push_back({state, newValue});
}

bool functionalUI::nextFrame()
{
	// this logic will fail to catch the case of multiple updates to the same state,
	// where the final update returns the state back to it's original value.
	// not critical though.
	bool changed = false;
	for (auto& update : updates)
	{
		changed |= !compareState(update.first.value, update.second); // where is the dirty flag set?
		update.first = update.second;
	}

	updates.clear();

	if (changed)
	{
		step();
	}

	return changed;
}

void functionalUI::step()
{
	// should advance 'frame number' state, then propagate changes through graph.
	//frameNumber = std::get<float>(frameNumber) + 1.0f;
	_RPT0(0, "-- FUNCTIONAL: STEP -------------\n");
	for (auto& n : graph)
	{
		if (n.dirty)
		{
			n.dirty = false;
			n(); // execute the node
		}
	}
}

void functionalUI::draw(Graphics& g)
{
	/* might be needed, probably not

	for (auto& n : renderNodes)
	{
		n(g);
	}
	*/

	// maybe output of final step should be a 'renderer' that can be called during painting.
	// so the graph's purpose is to reactivly produce a 'renderer' that does the actual drawing for the view.


	// should *not* process graph, only call renderer/s.
}

// set up a default patch. Not needed otherwise.
void functionalUI::init()
{
	// reference, what we're trying to achieve
#if 0
	Color color(Color::Bisque);
	auto brush = g.CreateSolidColorBrush(color);
	const Point center(100.0f, 100.0f);
	const float radius = 50.0f;
	//	g.FillCircle(center, radius, brush);

		// create circle geometry the hard way.
	auto geometry = g.GetFactory().CreatePathGeometry();
	{
		const float pi = M_PI;
		auto sink = geometry.Open();

		// make a circle from two half-circle arcs
		sink.BeginFigure({ center.x, center.y - radius }, FigureBegin::Filled);
		sink.AddArc({ { center.x, center.y + radius}, { radius, radius }, pi });
		sink.AddArc({ { center.x, center.y - radius }, { radius, radius }, pi });

		sink.EndFigure(FigureEnd::Closed);
		sink.Close();
	}

	g.FillGeometry(geometry, brush);
#endif

	///////////////////////////////////////////////////////////////////////////////////
	// NOTES.
	// need to have history of updates to the graph. could be immutable.
}

// initialize the factory
std::unordered_map<std::string, std::function<void(int32_t, Json::Value&, functionalUI::builder&)> > functionalUI::factory =
{
	{ "SE Solid Color Brush",
		[](int32_t handle, Json::Value& module_json, functionalUI::builder &builder) // , std::vector< std::unique_ptr<state_t> >& states, std::vector<node>& graph)
		{
			auto& states = builder.scheduler.states2;
			auto& nodegraph = builder.scheduler.graph;

			Color brushColor = Color::Black;

			ScanPinDefaults(module_json,
				[&brushColor](int idx, const std::string& value)
			{
				if (idx == 0)
				{
					brushColor = Color::FromHexStringU(value);
				}
			}
			);

			// Add state for input value (color)
//			const auto input1Idx = states.size();
			states.push_back(
				std::make_unique<observableState>(brushColor)
			);

			// Add function
			nodegraph.push_back(
				{
					[](std::vector<state_t*> statesx) -> state_data_t
					{
//						_RPT0(0, "SE Solid Color Brush: Execute\n");
						return vBrush(std::get<Color>(statesx[0]->value));
					},
					{states.back().get()},
					{ 0.0f },
					handle
				}
			);
		}
		},
		////////////////////////////////////////////////////////////////////////////////
{ "CD Circle",
	[](int32_t handle, Json::Value& module_json, functionalUI::builder &builder) // , std::vector< std::unique_ptr<state_t> >& states, std::vector<node>& graph)
	{
		auto& nodegraph = builder.scheduler.graph;

			float radius{ 10.f };
//			Point center{ 0.0f, 0.0f };

			ScanPinDefaults(module_json,
				[&radius](int idx, const std::string& value)
				{
					if (idx == 0) // circle has only one dimension
					{
						radius = std::stof(value);
					}
				}
			);
/*
			// TODO!!! this needs to come from pin default.
			// center
			const auto input1Idx = states.size();
			states.push_back(
				std::make_unique<state_t>(center)
			);

			// radius
			const auto input2Idx = states.size();
			states.push_back(
				std::make_unique<state_t>(radius)
			);
*/
			// 2. circle geometry
			nodegraph.push_back(
				{
				[](std::vector<state_t*> states) -> state_data_t
					{
						float radius{ 10.f };
						Point center{};

						if (states.size() > 0)
						{
							auto p = std::get_if<float>(&states[0]->value);
							if(p)
							{
								radius = *p;
							}
						}
						if (states.size() > 1)
						{
							auto p = std::get_if<Point>(&states[1]->value);
							if (p)
							{
								center = *p;
							}
						}

						//const auto& radius = std::get<float>(*states[0]);
						//const auto& center = std::get<Point>(*states[1]);
						radius = (std::max)(0.0f, radius);

						return vCircleGeometry(center, radius);
					},
					{
						//states[input1Idx].get(),
						//states[input2Idx].get()
					},
					{0.0f}, // result
					handle
				}
			);
		}
			},
			////////////////////////////////////////////////////////////////////////////////
		{ "CD Square",
[](int32_t handle, Json::Value& module_json, functionalUI::builder &builder) // , std::vector< std::unique_ptr<state_t> >& states, std::vector<node>& graph)
{
	auto& states = builder.scheduler.states2;
//	auto& graph = scheduler.graph;

	float size{ 10.f };
	Point center{ 0.0f, 0.0f };

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
	const auto input1Idx = states.size();
	states.push_back(
		std::make_unique<observableState>(size)
	);

	// center
	const auto input2Idx = states.size();
	states.push_back(
		std::make_unique<observableState>(center)
	);

	builder.scheduler.graph.push_back(
		{
		[](std::vector<state_t*> states) -> state_data_t
			{
//				const auto& size = std::get<float>(*states[0]);
//				const auto& center = std::get<Point>(*states[1]);

				float defaultfloat{};

				float* s = {};
				if (states.size() > 0)
				{
					s = std::get_if<float>(&states[0]->value);
				}

				if (!s)
				{
					s = &defaultfloat;
				}

				Point defaultPoint{ 0.0f, 0.0f };

				Point* p = {};
				if (states.size() > 1)
				{
					p = std::get_if<Point>(&states[1]->value);
				}

				if (!p)
				{
					p = &defaultPoint;
				}

				return vSquareGeometry(*p, *s);
			},
			{
				states[input1Idx].get(),
				states[input2Idx].get()
			},
			{0.0f}, // result
			handle
		}
	);
}
		},
////////////////////////////////////////////////////////////////////////////////
{ "CD Point2Values",
[](int32_t handle, Json::Value& module_json, functionalUI::builder &builder) // , std::vector< std::unique_ptr<state_t> >& states, std::vector<node>& graph)
{
//	auto& states = scheduler.states2;
	auto& nodegraph = builder.scheduler.graph;

	nodegraph.push_back(
		{
		[](std::vector<state_t*> states) -> state_data_t
			{
				Point defaultPoint{ 0.0f, 0.0f };

				Point* p = {};
				if (states.size() > 0)
				{
					p = std::get_if<Point>(&states[0]->value);
				}

				if (!p)
				{
					p = &defaultPoint;
				}

				return p->x;
			},
			{}, // inputs
			{0.0f}, // output
			handle
		}
	);
}
},
////////////////////////////////////////////////////////////////////////////////
{ "CD Value", // aka PatchMem
[](int32_t handle, Json::Value& module_json, functionalUI::builder &builder) // , std::vector< std::unique_ptr<state_t> >& states, std::vector<node>& graph)
{
	//!!! TODO use techniques from "CD Pointer" to slash this to nothing.
	//! // will need to proxy both input and output nodes.

//	auto& states = builder.scheduler.states2;
	auto& nodegraph = builder.scheduler.graph;

	// Add an input state to hold incoming parameter value.
	const auto stateIndex = builder.scheduler.registerInputParameter(float{});
	// Redirect 'from' connections to the proxy.
	const int pinIdx = 1; // Patch-Mem output pin.
	builder.connectionProxies.push_back({ handle, pinIdx, stateIndex });

	// register the link between the parameter and the state
	auto patchManager = builder.patchManager;
	const auto parameterHandle = patchManager->getParameterHandle(handle, 0);
	builder.parameterHandles.insert({ parameterHandle , static_cast<int32_t>(stateIndex) });

#if 1 // hmm, TODO
	const auto tempHandle = builder.getTempHandle();

	// add the 'Value' node. This will 'listen' on the graph for parameter updates (from the user).
	nodegraph.push_back(
		{
		[stateIndex, patchManager, parameterHandle](std::vector<state_t*> states) -> state_data_t
			{
				if (states.size() > 0)
				{
					auto floatp = std::get_if<float>(&states[0]->value);
					if (floatp)
					{
//						_RPTN(0, "setParameterValue(%f)\n", *floatp);
// stack overflow, calls step()	
					patchManager->setParameterValue({*floatp}, parameterHandle);
						// better?
						// scheduler.UpdateState(stateIndex, { *floatp }); // and have it figure out that that state is a parameter
					}
				}

				return {}; // doesn't output anything useful.
			},
		{},
			{0.0f}, // result
		tempHandle
		}
	);

	builder.connectionProxiesNode.push_back({ handle, tempHandle });
#endif
#if 0
	return;

	// Add a state to hold the parameter value
	const auto input1Idx = states.size();
	states.push_back(
		std::make_unique<state_t>(float{})
	);

	// add the 'Value' node, setting the new state as it's only argument.
	nodegraph.push_back(
		{

		[](std::vector<state_t*> states) -> state_t
			{
				// support custom calling from 'outside' with one float 'state' as a way to get parameters into the graph.
		// !!! confusing, will be overwritten by any user-made connections !!!
		// probly need to diferentiate patch-value and input-value (from graph)
				if (states.size() > 0)
				{
					auto floatp = std::get_if<float>(states[0]);
					if (floatp)
					{
						return *floatp;
					}
				}

				return float{};
			},
			{
				// insert a state to hold the patch value (ref: SubViewCadmium::setParameter)
				states[input1Idx].get(),
			},
			0.0f,
			handle
		}
	);
#endif
}
},
////////////////////////////////////////////////////////////////////////////////
{ "CD Pointer", // aka Mouse position
[](int32_t handle, Json::Value& module_json, functionalUI::builder& builder)
{
	const auto stateIndex = builder.scheduler.registerInputState("pointerPosition", Point{});

	builder.connectionProxies.push_back({ handle, 0, stateIndex });
}
},
////////////////////////////////////////////////////////////////////////////////
{ "SE Render", // should be called fill-geometry or suchlike
[](int32_t handle, Json::Value& module_json, functionalUI::builder &builder)
{
//	auto& states = scheduler.states2;
	auto& nodegraph = builder.scheduler.graph;

	nodegraph.push_back(
		{
			[](std::vector<state_t*> states) -> state_data_t
			{
				// probably nicer if framework provides a default object, rather than endless checking for null
				vBrush* brush = {};
				if (states.size() > 0)
				{
					brush = std::get_if<vBrush>(&states[0]->value);
				}

				vGeometry* geometry = {};
				if (states.size() > 1)
				{
					geometry = std::get_if<vCircleGeometry>(&states[1]->value);
					if (!geometry)
					{
						geometry = std::get_if<vSquareGeometry>(&states[1]->value);
					}
				}

                // this technique pre-calculates the variant 'get's just once. could apply this to ALL nodes? for extra efficiency.
                return RendererX(
                {
                    [brush, geometry](Graphics& g) -> void
                    {
                        if (geometry && brush) // try to screen the need for this out
                        {
                            g.FillGeometry(geometry->native(g), brush->native(g));
                        }
                    }
                }
            );
        },
        {},
        {0.0f}, // result
        handle
    }
);
	// note down the render nodes, because we need to provide them with the graphics context later.
	builder.renderNodeHandles.push_back(handle);
}
},

////////////////////////////////////////////////////////////////////////////////
{ "CD Apply", // AKA 'EVAL'
[](int32_t handle, Json::Value& module_json, functionalUI::builder& builder)
{
		auto& nodegraph = builder.scheduler.graph;

		nodegraph.push_back(
			{
			[](std::vector<state_t*> states) -> state_data_t
				{
					// https://www.lurklurk.org/cpp_clos.html 'The Lisp Interpreter'
					// https://kirit.com/Build%20me%20a%20LISP 'Build me a LISP'
					if (states.size() > 0)
					{
						if (auto list = std::get_if<List>(&states[0]->value))
						{
							if (list->size() > 0)
							{
								auto& listr = *list;
								auto& s = listr[0].value;
								auto functionToApply = std::get_if<list_function>(&s);

								if (functionToApply)
								{
									List remainderOfList;
									for (int i = 1; i < listr.size(); ++i)
									{
										remainderOfList.push_back(listr[i].value);
									}

									// return (*functionToApply)(remainderOfList).value;
         
                                    return apply_list_function(*functionToApply, remainderOfList).value;
								}
							}

							//for (auto& v : *list)
							//{
							//	_RPTN(0, "%d ", v.value.index());
							//}
						}
						//_RPT0(0, "\n");
					}

					return {};
				},
				{}, // inputs
				{0.0f}, // output
				handle
			}
		);
	}
},
////////////////////////////////////////////////////////////////////////////////
	{ "CD Make List",
	[](int32_t handle, Json::Value& module_json, functionalUI::builder& builder)
	{
			auto& nodegraph = builder.scheduler.graph;

			nodegraph.push_back(
				{
				[](std::vector<state_t*> states) -> state_data_t
					{
						List r;

						if (states.size() > 0)// cope with 'first one null' bug due to output pin being first.
						{
							// bug here is that it does return correct list, but shallow comparision results in no 'dirty' flag set on downstream modules
							// becuase the contents are pointers to states, where the addresses of the state have not changed (but their *value* has).
	//						return std::vector<state_t>(states.begin() + 1, states.end()); // cope with 'first one null' bug due to output pin being first.
	//						for (auto& v : states)
							for (auto it = states.begin() + 1; it != states.end(); ++it)
							{
								auto& v = *it;
								r.push_back(v->value);
							}
						}
						return r;
					},
					{}, // inputs
					{0.0f}, // output
					handle
				}
			);
		}
	},
		////////////////////////////////////////////////////////////////////////////////
		{ "CD Render Function",
		[](int32_t handle, Json::Value& module_json, functionalUI::builder& builder)
		{
			auto& nodegraph = builder.scheduler.graph;

			nodegraph.push_back(
				{
				[](std::vector<state_t*> states) -> state_data_t
					{
						return list_function([](const List& list) -> state_data_t
							{
#if 0 // test printing out arguments
								for (auto& v : list)
								{
									_RPTN(0, "%d ", v.value.index());
								}
								_RPT0(0, "\n");

								return {0.0f};
#endif
								if (list.size() < 2)
									return {};

								// probably nicer if framework provides a default object, rather than endless checking for null
								auto brush = std::get_if<vBrush>(&list[0].value);

								// only works with circles at present.
//								vGeometry* geometry = {};
								auto geometry = std::get_if<vCircleGeometry>(&list[1].value);
/*
								if (!geometry)
								{
									geometry = std::get_if<vSquareGeometry>(&list[1].value);
								}
*/

								if (!brush || !geometry)
								{
									return {};
								}

								// gotta pass by value into function, else they are only temporaries.
								auto& brushc = *brush;
								auto& geomc = *geometry;

								// this technique pre-calculates the variant 'get's just once. could apply this to ALL nodes? for extra efficiency.
								return RendererX(
									{
										[brushc, geomc](Graphics& g) -> void
										{

									// !!! problem: CRASH!!! geometry was a (deleted) temporary object.
									// they probably all are.
//											if (geometry && brush) // try to screen the need for this out
											{
												g.FillGeometry(geomc.native(g), brushc.native(g));
											}
										}
									}
								);
							}
                        );
					},
					{}, // inputs
					{0.0f}, // output
					handle
				}
			);
			}
		},
// new one here

};
