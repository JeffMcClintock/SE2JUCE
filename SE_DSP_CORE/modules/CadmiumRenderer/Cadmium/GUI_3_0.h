#include "Drawing.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <variant>
#include <functional>
#include "../se_sdk3/TimerManager.h"

#include "../jsoncpp/json/json.h"

inline void ScanPinDefaults(
	Json::Value& module_json,
	std::function < void(int, const std::string&) > callback
)
{
	int index = 0;
	for (auto& pin_json : module_json["Pins"])
	{
		callback(index, pin_json["default"].asString());
		++index;
	}
}

struct RendererX
{
	std::function < void(GmpiDrawing::Graphics&) > function;
/*
	RendererX(std::function < void(GmpiDrawing::Graphics&) > pfunction) :
		function(function)
	{}
*/
};

class vBrush
{
	mutable GmpiDrawing::SolidColorBrush brush;
	GmpiDrawing::Color color;

public:
	vBrush(GmpiDrawing::Color c) : color(c)
	{}

	// Copy constructor
	vBrush(const vBrush& other)
	{
		// had to cast away constness to access D2D brush.
		// this could be a problem were we not treating the bush as immutable anyhow.
		// much better would be a GimpiDrawing: Mutable Bush class and Immutable Brush class !!!!!
		// maybe the immutable one shares the underlying object, does not expose setcolor() etc
		// mayby a simpler workarround would be delclare brush mutable
		brush = const_cast<GmpiDrawing::SolidColorBrush&>(other.brush).Get();
		color = other.color;
	}

	vBrush& operator=(const vBrush& rhs)
	{
		brush = const_cast<GmpiDrawing::SolidColorBrush&>(rhs.brush).Get();
		color = rhs.color;

		return *this;
	}

	// hacky should return a read-only object
	GmpiDrawing::SolidColorBrush& native(GmpiDrawing::Graphics& g) const
	{
		// TODO cache it based on factory.
		brush = g.CreateSolidColorBrush(color);
		return brush;
	}
};

class vGeometry
{
public:
	virtual GmpiDrawing::PathGeometry& native(GmpiDrawing::Graphics& g) const = 0;
};

class vCircleGeometry : public vGeometry
{
	mutable GmpiDrawing::PathGeometry geometry;

	GmpiDrawing::Point center;
	float radius;

public:
	vCircleGeometry(GmpiDrawing::Point c, float r) : center(c), radius(r)
	{}

	// Copy constructor
	vCircleGeometry(const vCircleGeometry& other)
	{
		// had to cast away constness to access D2D brush. (mac only)
		// this could be a problem were we not treating the bush as immutable anyhow.
		// much better would be a GimpiDrawing: Mutable Bush class and Imutable Brush class !!!!!
		// maybe the immutable one shares the underlying object, does not expose setcolor() etc
		// mayby a simpler workarround would be delclare brush mutable
		geometry = const_cast<GmpiDrawing::PathGeometry&>(other.geometry).Get();
		center = other.center;
		radius = other.radius;
	}

	vCircleGeometry& operator=(const vCircleGeometry& rhs)
	{
		geometry = const_cast<GmpiDrawing::PathGeometry&>(rhs.geometry).Get();
		center = rhs.center;
		radius = rhs.radius;

		return *this;
	}

	GmpiDrawing::PathGeometry& native(GmpiDrawing::Graphics& g) const override
	{
		// TODO cache it based on factory.
		geometry = g.GetFactory().CreatePathGeometry();
		const float pi = M_PI;
		auto sink = geometry.Open();

		// make a circle from two half-circle arcs
		sink.BeginFigure({ center.x, center.y - radius }, GmpiDrawing::FigureBegin::Filled);
		sink.AddArc({ { center.x, center.y + radius}, { radius, radius }, pi });
		sink.AddArc({ { center.x, center.y - radius }, { radius, radius }, pi });

		sink.EndFigure(GmpiDrawing::FigureEnd::Closed);
		sink.Close();
		return geometry;
	}
};

class vSquareGeometry : public vGeometry
{
	mutable GmpiDrawing::PathGeometry geometry;

	GmpiDrawing::Point center;
	float size;

public:
	vSquareGeometry(GmpiDrawing::Point c, float s) : center(c), size(s)
	{}

	// Copy constructor
	vSquareGeometry(const vSquareGeometry& other)
	{
		// had to cast away constness to access D2D brush. (mac only)
		// this could be a problem were we not treating the bush as immutable anyhow.
		// much better would be a GimpiDrawing: Mutable Bush class and Imutable Brush class !!!!!
		// maybe the immutable one shares the underlying object, does not expose setcolor() etc
		// mayby a simpler workarround would be declare brush mutable
		geometry = const_cast<GmpiDrawing::PathGeometry&>(other.geometry).Get();
		center = other.center;
		size = other.size;
	}

	vSquareGeometry& operator=(const vSquareGeometry& rhs)
	{
		geometry = const_cast<GmpiDrawing::PathGeometry&>(rhs.geometry).Get();
		center = rhs.center;
		size = rhs.size;

		return *this;
	}

	GmpiDrawing::PathGeometry& native(GmpiDrawing::Graphics& g) const override
	{
		// TODO cache it based on factory.
		geometry = g.GetFactory().CreatePathGeometry();
		auto sink = geometry.Open();

		const float halfSize = size * 0.5f;
		const float left = center.x - halfSize;
		const float right = center.x + halfSize;
		const float top = center.y - halfSize;
		const float bottom = center.y + halfSize;
		sink.BeginFigure({ left, top }, GmpiDrawing::FigureBegin::Filled);
		sink.AddLine({ right, top});
		sink.AddLine({ right, bottom });
		sink.AddLine({ left, bottom });

		sink.EndFigure(GmpiDrawing::FigureEnd::Closed);
		sink.Close();
		return geometry;
	}
};


struct node;
struct state_t;

// a list of values (probly don't want to lug arround 'downstreamNodes')
using List = std::vector<state_t>;

// a function taking a list of values
// using list_function = std::function<state_t(List&)>;

class list_function
{
public:
    std::function<state_t(const List&)> function;
    
    list_function(std::function<state_t(const List&)>);

    bool operator==(const list_function& other);
};

using state_data_t = std::variant<
	List,		// leave List first (ref compareState())
    float,
    GmpiDrawing::Point,
    GmpiDrawing::Color,
    vCircleGeometry,
    vSquareGeometry,
    vBrush,
	list_function,
    RendererX
>;

using test_t = std::variant<
    List,        // leave List first (ref compareState())
    float,
    GmpiDrawing::Point,
GmpiDrawing::Color,
vCircleGeometry,
vSquareGeometry,
vBrush,
//list_function,
RendererX
>;

const static char* typeNames[] =
{
	"List",
	"float",
	"GmpiDrawing::Point",
	"GmpiDrawing::Color",
	"vCircleGeometry",
	"vSquareGeometry",
	"vBrush",
	"list_function",
	"RendererX"
};

struct state_t
{
    state_t(){}
    state_t(state_data_t pvalue) : value(pvalue) {}

    state_data_t value;
};

// depends on circular definitions of 'state_t' and 'list_function' and 'List'
// so it's here rather than a member of list_function
state_t apply_list_function(list_function, const List&);


struct observableState : public state_t
{
	observableState(state_data_t pvalue) : state_t(pvalue) {}

	observableState& operator=(const state_data_t& newvalue);

	std::vector<node*> downstreamNodes;
};

struct node
{
    node(
            std::function < state_data_t(std::vector<state_t*>) > pfunction,
            std::vector<state_t*> parguments,
			state_data_t presult,
            int32_t phandle
        ) :
            function(pfunction),
            arguments(parguments),
            result(presult),
            handle(phandle)
    {
    }

    bool dirty = true;
    std::function < state_data_t(std::vector<state_t*>) > function;
    std::vector<state_t*> arguments;
	observableState result; // maybe could support multiple outputs: std::vector<state_t> results;
    int32_t handle = -1;
	std::string debug_name;

	void operator()();
};

class functionalUI
{
public:

	struct builder
	{
		int32_t getTempHandle()
		{
			static int32_t nextTempHandle = -1;
			return nextTempHandle--;
		}

		struct connectionProxy
		{
			int32_t moduleHandle;
			int32_t pinId;
			size_t stateIndex;
		};

		functionalUI& scheduler;
		class IGuiHost2* patchManager = {};
		std::unordered_map<int32_t, int32_t> parameterHandles; // parameter-handle, state index
		std::vector<connectionProxy> connectionProxies; // states taking place of node output. (patch memory)
		std::vector< std::pair<int32_t, int32_t> > connectionProxiesNode;
		std::vector<int32_t> renderNodeHandles;
	};

	static std::unordered_map<std::string, std::function<void(int32_t, Json::Value&, functionalUI::builder& /*, std::vector< std::unique_ptr<functionalUI::state_t> >&, std::vector<functionalUI::node>& */)> > factory;

#if 0
	// !!! could the rendernode just be a normal node? !!!
	// we would need to pass in GmpiDrawing::Graphics as a special 'vGraphics' argument.

	struct renderNode
	{
//		std::function < state_t(std::vector<state_t*>, GmpiDrawing::Graphics& g) > function;
		std::function < void(std::vector<state_t*>, GmpiDrawing::Graphics& g) > function;
		std::vector<state_t*> arguments;
//		state_t result;

		void operator()(GmpiDrawing::Graphics& g)
		{
			/*result = */ function(arguments, g);
		}
	};
#endif

	std::vector<node> graph;
	/* might be needed, probably not
		std::vector<renderNode> renderNodes;
*/
	std::vector< std::unique_ptr<observableState> > states2; // allows push/realloc without altering address of state.
	std::vector < std::pair<observableState&, state_data_t> > updates;

	void init();
	bool nextFrame();
	void draw(GmpiDrawing::Graphics& g);
	void step();

	void updateState(observableState& state, state_data_t newValue);

	struct inputInfo
	{
		int stateIndex;
		int32_t moduleHandle;
		int modulePinId;
	};

	std::unordered_map< std::string, size_t > inputStates;

	size_t registerInputState(std::string name, state_data_t initialValue)
	{
		auto it = inputStates.find(name);
		if (it != inputStates.end())
		{
			assert(states2[it->second]->value.index() == initialValue.index()); // same type?

			return it->second;
		}

		const auto stateIndex = states2.size();
		states2.push_back(std::make_unique<observableState>(initialValue));
		inputStates[name] = stateIndex;

		return stateIndex;
	}

	size_t registerInputParameter(state_data_t initialValue)
	{
		const auto stateIndex = states2.size();
		states2.push_back(std::make_unique<observableState>(initialValue));
		return stateIndex;
	}

	void onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		auto it = inputStates.find("pointerPosition");
		if (it != inputStates.end())
		{
			updateState(*states2[it->second], GmpiDrawing::Point(point));
		}
	}
};
