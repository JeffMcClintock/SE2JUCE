#include "./PolyphonyControlGui.h"
#include "../shared/voice_allocation_modes.h"

using namespace gmpi;
using namespace gmpi_gui;

using namespace voice_allocation;
using namespace voice_allocation::bits;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, PolyphonyControlGui1, L"SE Polyphony Control");
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, PolyphonyControlGui2, L"SE Polyphony Control2");
SE_DECLARE_INIT_STATIC_FILE(PolyphonyControl_Gui)

PolyphonyControlGui1::PolyphonyControlGui1()
{
	initializePin(hostPolyphony, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetHostPolyphony ));
	initializePin(hostReserveVoices, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetHostPolyphonyReserve ));
	initializePin(hostVoiceAllocationMode, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui1::onSetHostVoiceAllocationMode ));

	initializePin(polyphony, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetPolyphony ));
	initializePin(itemList3);
	initializePin(reserveVoices, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetPolyphonyReserve ));
	initializePin(itemList4);
	initializePin(voiceAllocationMode, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui1::onSetVoiceAllocationMode ));
	initializePin(itemList_voiceAllocationMode);

	initializePin(monoNotePriority, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetNotePriority ));
	initializePin(itemList2);

/*
	hostPolyphony.initialize(			this, 0, static_cast<MpGuiBaseMemberPtr>(&PolyphonyControlGui_base::onSetHostPolyphony) );
	hostReserveVoices.initialize(		this, 1, static_cast<MpGuiBaseMemberPtr>(&PolyphonyControlGui_base::onSetHostPolyphonyReserve) );
	hostVoiceAllocationMode.initialize(	this, 2, static_cast<MpGuiBaseMemberPtr>(&PolyphonyControlGui_base::onSetHostVoiceAllocationMode) );

	polyphony.initialize(				this, 3, static_cast<MpGuiBaseMemberPtr>(&PolyphonyControlGui_base::onSetPolyphony) );
	itemList3.initialize(				this, 4 );
	reserveVoices.initialize(			this, 5, static_cast<MpGuiBaseMemberPtr>(&PolyphonyControlGui_base::onSetPolyphonyReserve) );
	itemList4.initialize(				this, 6 );
	voiceAllocationMode.initialize(		this, 7, static_cast<MpGuiBaseMemberPtr>(&PolyphonyControlGui_base::onSetVoiceAllocationMode) );
	itemList.initialize(				this, 8 );
	notePriority.initialize(this, 9, static_cast<MpGuiBaseMemberPtr>( &PolyphonyControlGui_base::onSetNotePriority ));
	itemList2.initialize(this, 10);
	GlideType.initialize(this, 11, static_cast<MpGuiBaseMemberPtr>( &PolyphonyControlGui_base::onSetGlide ));
	itemList_GlideType.initialize(this, 12);
	GlideTiming.initialize(this, 13, static_cast<MpGuiBaseMemberPtr>( &PolyphonyControlGui_base::onSetGlideTiming ));
	itemList_GlideTiming.initialize(this, 14);

	hostBendRange.initialize(this, 15, static_cast<MpGuiBaseMemberPtr>( &PolyphonyControlGui_base::onSetHostBendRange ));
	BendRange.initialize(this, 16, static_cast<MpGuiBaseMemberPtr>( &PolyphonyControlGui_base::onSetBendRange ));
	itemList_BendRange.initialize(this, 17);

	host_PortamentoTime.initialize(this, 18, static_cast<MpGuiBaseMemberPtr>( &PolyphonyControlGui_base::onSetHostPortamento ));
	PortamentoTime.initialize(this, 19, static_cast<MpGuiBaseMemberPtr>( &PolyphonyControlGui_base::onSetPortamento ));
	*/
}

PolyphonyControlGui2::PolyphonyControlGui2()
{
	initializePin(hostPolyphony, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetHostPolyphony ));
	initializePin(hostReserveVoices, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetHostPolyphonyReserve ));
	initializePin(hostVoiceAllocationMode, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetHostVoiceAllocationMode ));
	initializePin(host_PortamentoTime, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetHostPortamento ));
	initializePin(hostBendRange, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetHostBendRange ));


	initializePin(polyphony, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetPolyphony ));
	initializePin(itemList3);
	initializePin(reserveVoices, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetPolyphonyReserve ));
	initializePin(itemList4);
	initializePin(voiceStealMode, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetVoiceStealMode ));
	initializePin(itemList_voiceSteal);


	initializePin(monoMode, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetMonoMode ));
	initializePin(monoRetrigger, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetMonoMode ));
	initializePin(monoNotePriority, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui_base::onSetNotePriority ));
	initializePin(itemList2);

	initializePin(PortamentoTime, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetPortamento ));
	initializePin(GlideType, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetGlide ));
	initializePin(itemList_GlideType);
	initializePin(GlideTiming, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetGlideTiming));
	initializePin(itemList_GlideTiming);

	initializePin(BendRange, static_cast<MpGuiBaseMemberPtr2>( &PolyphonyControlGui2::onSetBendRange ));
	initializePin(itemList_BendRange);

	initializePin(VoiceRefresh, static_cast<MpGuiBaseMemberPtr2>(&PolyphonyControlGui2::onSetVoiceRefresh));
	initializePin(itemList_VoiceRefresh);
}

int32_t PolyphonyControlGui_base::initialize()
{
	itemList2 = L"Off,Low,High,Last";
	itemList3 = L"range 1,128";
	itemList4 = L"range 0,128";

	return MpGuiInvisibleBase::initialize();
}

int32_t PolyphonyControlGui1::initialize()
{
	itemList_voiceAllocationMode = L"Poly,Poly (Hard),Poly (Overlap), Mono=4, Mono (Retrigger)";

	return PolyphonyControlGui_base::initialize();
}

int32_t PolyphonyControlGui2::initialize()
{
	itemList_voiceSteal = L"Soft,Hard,Overlap";
	itemList_GlideType = L"Legato, Always";
	itemList_GlideTiming = L"Const Rate, Const Time";

	itemList_BendRange = L"0,1,2,3,4,5,6,7,8,9,10,11,12";
	itemList_VoiceRefresh = L"Enable, Disable";

	return PolyphonyControlGui_base::initialize();
}

void PolyphonyControlGui2::onSetBendRange()
{
	hostBendRange = (float)BendRange;
}

void PolyphonyControlGui2::onSetHostBendRange()
{
	BendRange = (int) (hostBendRange + 0.5f);
}

void PolyphonyControlGui2::onSetPortamento()
{
	host_PortamentoTime = PortamentoTime;
}

void PolyphonyControlGui2::onSetHostPortamento()
{
	PortamentoTime = host_PortamentoTime;
}

void PolyphonyControlGui_base::onSetPolyphony()
{
	hostPolyphony = polyphony;
}

void PolyphonyControlGui_base::onSetHostPolyphony()
{
	polyphony = hostPolyphony;
}

void PolyphonyControlGui_base::onSetPolyphonyReserve()
{
	hostReserveVoices = reserveVoices;
}

void PolyphonyControlGui_base::onSetHostPolyphonyReserve()
{
	reserveVoices = hostReserveVoices;
}

void PolyphonyControlGui1::onSetVoiceAllocationMode()
{
	auto v = hostVoiceAllocationMode.getValue();

	hostVoiceAllocationMode = ( hostVoiceAllocationMode & 0xffffff00 ) | voiceAllocationMode;

	assert(hostVoiceAllocationMode == insertBits(v, 0, 8, voiceAllocationMode));
}

void PolyphonyControlGui2::onSetMonoMode()
{
	const int flags
		= MM_IN_USE
		| (monoMode.getValue() ? MM_ON : 0)
		| (monoRetrigger.getValue() ? MM_RETRIGGER : 0);

	const auto v = hostVoiceAllocationMode.getValue();
	hostVoiceAllocationMode = insertBits(v, MonoModes_startbit, MonoModes_sizebits, flags);
}

void PolyphonyControlGui2::onSetVoiceStealMode()
{
	// 	L"Poly,Poly (Hard),Poly (Overlap), Mono=4, Mono (Retrigger)";
	auto v = hostVoiceAllocationMode.getValue();
/*
	int combinedVoiceAllocationMode;
	if( monoMode )
	{
		if( monoRetrigger )
		{
			combinedVoiceAllocationMode = 5; // Mono(Retrigger)
		}
		else
		{
			combinedVoiceAllocationMode = 4; // Mono(No Retrigger)
		}
	}
	else
	{
		combinedVoiceAllocationMode = voiceStealMode;
	}
*/
	const int combinedVoiceAllocationMode = voiceStealMode & 0x03;

	hostVoiceAllocationMode = ( hostVoiceAllocationMode & 0xffffff00 ) | combinedVoiceAllocationMode;

	assert(hostVoiceAllocationMode == insertBits(v, 0, 8, combinedVoiceAllocationMode));
}

void PolyphonyControlGui_base::onSetNotePriority()
{
	const auto v = hostVoiceAllocationMode.getValue();
	hostVoiceAllocationMode = insertBits(v, NotePriority_startbit, NotePriority_sizebits, monoNotePriority);
}

void PolyphonyControlGui2::onSetVoiceRefresh()
{
	auto v = hostVoiceAllocationMode.getValue();

	const int bitPosistion = 19;
	const int mask = ~(1 << bitPosistion);
	hostVoiceAllocationMode = (hostVoiceAllocationMode & mask) | ((VoiceRefresh & 0x01) << bitPosistion);

	assert(hostVoiceAllocationMode == insertBits(v, 19, 1, VoiceRefresh));
}

void PolyphonyControlGui2::onSetGlide()
{
	auto v = hostVoiceAllocationMode.getValue();

// should be 0xfffeffff	hostVoiceAllocationMode = ( hostVoiceAllocationMode & 0xfffcffff ) | ( GlideType << 16 );

//	assert(hostVoiceAllocationMode == insertBits(v, 16, 1, GlideType));

	hostVoiceAllocationMode = insertBits(v, 16, 1, GlideType);
}

void PolyphonyControlGui2::onSetGlideTiming()
{
	auto v = hostVoiceAllocationMode.getValue();

	hostVoiceAllocationMode = ( hostVoiceAllocationMode & 0xfffbffff ) | ( GlideTiming << 18 );

	assert(hostVoiceAllocationMode == insertBits(v, 18, 1, GlideTiming));
}

void PolyphonyControlGui1::onSetHostVoiceAllocationMode()
{
	voiceAllocationMode = 0xff & hostVoiceAllocationMode;
	monoNotePriority = 0xff & ( ( (int)hostVoiceAllocationMode ) >> 8 );

	auto v = hostVoiceAllocationMode.getValue();

	assert(voiceAllocationMode == extractBits(v, 0, 8));
	assert(monoNotePriority == extractBits(v, 8, 8));
}

void PolyphonyControlGui2::onSetHostVoiceAllocationMode()
{
	const auto allocationMode = hostVoiceAllocationMode.getValue();

	monoNotePriority = 0x03 & ( ( (int)hostVoiceAllocationMode ) >> 8 );
	GlideType = 0x01 & ( ( (int)hostVoiceAllocationMode ) >> 16 );
	GlideTiming = 0x01 & ( ( (int)hostVoiceAllocationMode ) >> 18 );

	voiceStealMode = 0x03 & ( (int)hostVoiceAllocationMode );

	monoMode = isMonoMode(allocationMode);
	monoRetrigger = isMonoRetrigger(allocationMode);

	{
		const int bitPosistion = 19;
		VoiceRefresh = 0x01 & (((int)hostVoiceAllocationMode) >> bitPosistion);
	}

	assert(monoNotePriority == extractBits(allocationMode, 8, 2));
	assert(GlideType == extractBits(allocationMode, 16, 1));
	assert(GlideTiming == extractBits(allocationMode, 18, 1));
	assert(voiceStealMode == extractBits(allocationMode, 0, 2));
	assert(VoiceRefresh.getValue() == extractBits(allocationMode, 19, 1));
}

