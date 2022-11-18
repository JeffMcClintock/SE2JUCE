#include "./Switches.h"
SE_DECLARE_INIT_STATIC_FILE(Switch)

typedef SwitchManyToOne<float> SwitchFloat;
typedef SwitchManyToOne<int> SwitchInt;
typedef SwitchManyToOne<bool> SwitchBool;
typedef SwitchManyToOne<MpBlob> SwitchBlob;
typedef SwitchManyToOne<std::wstring> SwitchText;

REGISTER_PLUGIN2(SwitchFloat, L"SE Switch > (Float)");
REGISTER_PLUGIN2(SwitchInt, L"SE Switch > (Int)");
REGISTER_PLUGIN2(SwitchBool, L"SE Switch > (Bool)");
REGISTER_PLUGIN2(SwitchBlob, L"SE Switch > (Blob)");
REGISTER_PLUGIN2(SwitchText, L"SE Switch > (Text)");

typedef SwitchOneToMany<float> SwitchFloatToMany;
typedef SwitchOneToMany<int> SwitchIntToMany;
typedef SwitchOneToMany<bool> SwitchBoolToMany;
typedef SwitchOneToMany<MpBlob> SwitchBlobToMany;
typedef SwitchOneToMany<std::wstring> SwitchTextToMany;

REGISTER_PLUGIN2(SwitchFloatToMany, L"SE Switch < (Float)");
REGISTER_PLUGIN2(SwitchIntToMany, L"SE Switch < (Int)");
REGISTER_PLUGIN2(SwitchBoolToMany, L"SE Switch < (Bool)");
REGISTER_PLUGIN2(SwitchBlobToMany, L"SE Switch < (Blob)");
REGISTER_PLUGIN2(SwitchTextToMany, L"SE Switch < (Text)");



