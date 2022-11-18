#pragma once
#include <vector>
#include <algorithm>

class UgDebugInfo
{
public:
	UgDebugInfo(class EventProcessor* module);
	void Process( int buffer_offset, int blocksize );
	void AddCpu( float clonesCpu, float cpuPeakCycles)
	{
		cpuMeasuedCycles += clonesCpu;
		cpuPeak = (std::max)(cpuPeak, cpuPeakCycles);
	}
	void CpuToGui();

	class ug_base* m_module;

private:
	void VerfyBlocksStart( int buffer_offset, int blocksize );
	void VerfyBlocksEnd( int buffer_offset, int blocksize );

	// easier to compare int as raw bytes (float has NANs etc which always compare 'false').
	std::vector<int> bufferEndValue;
	float cpuMeasuedCycles;
	float cpuPeak = 0.0f;
};
