#pragma once

// Posible notification messages for comunicating between objects
#define OM_REFRESH_PRESENTERS 6
#define OM_DRAG_NEW_MODULE    7
#define OM_DELETE			 8
#define OM_UPDATE_BROWSER_FILTER 9
#define OM_NAME_CHANGE		10
#define OM_SHOW_PROPERTIES  11
#define OM_ENUM_LIST_CHANGE	12
#define OM_DELETE_DOC		15
#define OM_ADD_CHILD		17

#define OM_REMOVE_CHILD		18 // temp. only used by controls-on-panel

#define OM_MUTE_CHANGE		19
#define OM_SCREENSHOT		21
#define OM_REFRESH_PARAMETERS	24
#define OM_LAYOUT_CHANGE2	25

// child plug has received a message thru it's wires from annother ug
#define OM_DOWNSTREAM_PLUG_ENUM_CHANGE	27
#define OM_PLUGS_CHANGE			29
#define OM_ONCHANGE_CHILD_SELECTED	 30
#define OM_ONCHANGE_CHILD_POSITION_STRUCT	32
#define OM_ONCHANGE_CHILD_COLOUR	33
#define OM_ONCHANGE_CHILD_POSITION_PANEL	34
#define OM_PATCH_AUTOMATOR_CHANGE	39
#define OM_LOCKED_CHANGE		44

// Messages sent not to GUI, but to other CUG objects.
// force list entry to change (used by soundfont player)
#define OM_DOWNSTREAM_PLUG_DEFAULT_CHANGE	47
#define OM_UPSTREAM_PLUG_CONNECT		48
#define OM_UPSTREAM_PLUG_DISCONNECT	49

#define OM_CHILD_TO_FRONT		54
#define OM_CHILD_TO_BACK		55

#define OM_DISCONNECT_IN_GUI_CONNECTIONS 56
#define OM_GUI_PLUG_DEFAULT_CHANGE		57
#define OM_NEW_IN_GUI_CONNECTION		58
#define OM_RELOAD_VIEWS					59
#define OM_ON_DSP_MESSAGE				60

// child plug has solid first-time connection
#define OM_DOWNSTREAM_PLUG_CONNECT2		61
#define OM_CPU_UPDATE					63

// Update presets. Arg is handle of container.
#define OM_WPF_UPDATE_PRESET_BROWSER	64
#define OM_WPF_REPLACE_DIALOG			65
#define OM_WPF_UPDATE_EDIT_MENU			66
#define OM_UPDATE_MODULE_BROWSER		67

#define OM_USER_1						2000 // thru 3000. UG CAN USE THIS FOR ANYTHING IT WANTS
