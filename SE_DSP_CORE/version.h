#pragma once

// Synthed exe version number
// SE 1.0 accidentally got version # 1.1, SE 1.1 is V111000
// Little practical use. No longer updated automatically. Update only for major version changes.
#define EXE_VERSION_NUM 150000

//version 1.2 beta starts at 118000, 1.1 can use up to 117999
// 120690
// 120910 : 24/07/2013 VST3 GUIDs changed to string from wstring.
// 120960 : 27/08/2013 internal Inverter replaced with External SEM.
// 121470 : 16/10/2014 PatchParameter new field sharedParamPluginId_.
// 121670 : 28/01/2015 PatchParameter new field vstDisplayType_.
// 130000 : 04/05/2017 moduleinfo latency added	.
// 130010 : 15/09/2017 SEM cache: Added Mac SEM path.
// NOTE: Due to bug, files created with V130000 - V130010 are not interchangeable between 1.2 and 1.3
// 130020 : 10/06/2017 No Change in 1.3 (Fix bug in V1.2 not archiving module latency).
// 130030 : 30/01/2018 controller pins added to moduleinfo.
// 130040 : 02/02/2018 Parameter metadata now saved in module scan database.
// 130041 : Project has been saved from at least most recent SE V1.4 to ensure all obsolete unsupported modules are removed.
#define FILE_FORMAT_VERSION_NUM 130041

//#define COMPILE_DEMO_VERSION
#define SYNTHEDIT_REGISTRY_KEY L"SYNTHEDIT_X64_12"
