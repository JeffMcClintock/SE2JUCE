#include "./Waveshaper2.h"

SE_DECLARE_INIT_STATIC_FILE(Waveshaper2);

REGISTER_PLUGIN( Waveshaper2, L"SynthEdit Waveshaper3" );

using namespace std;

// Line-segment based.
void Waveshaper2::FillLookupTable()
{
	float nodes[2][WS_NODE_COUNT]; // x,y co-ords of control points

	wstring s = pinShape;

	// convert CString of numbers to array of screen co-ords
	int i = 0;
	while( s.length() > 0 && i < WS_NODE_COUNT )
	{
		int p1 = (int) s.find(L"(");
		int p2 = (int) s.find(L",");
		int p3 = (int) s.find(L")");
		if( p3 < 1 )
			return;

		wchar_t* temp;	// convert string to float
		float x = (float) wcstod( s.substr(p1+1,p2-p1-1).c_str(), &temp );
		float y = (float) wcstod( s.substr(p2+1,p3-p2-1).c_str(), &temp );
		x = ( x + 5.f ) * 10.f; // convert to screen co-ords
		y = ( -y + 5.f ) * 10.f;

		nodes[0][i] = x;
		nodes[1][i++] = y;

		p3++;
		s = s.substr(p3, s.length() - p3); //.Right( s.length() - p3 - 1);
	}

	int segments = 11;
	double from_x = 0.f; // first x co=ord
	int table_index = 0;
	for( int i = 1 ; i < segments; ++i )
	{
		double to_x = nodes[0][i] * TABLE_SIZE * 0.01;
		double delta_y = nodes[1][i] - nodes[1][i-1];
		double delta_x = from_x - to_x;
		double slope = 0.01 * delta_y / delta_x; 
		double c = 0.5 - 0.01 * nodes[1][i-1] - from_x * slope; 

		int int_to = (int) to_x;

		if( int_to > TABLE_SIZE )
			int_to = TABLE_SIZE;

		if( i == segments - 1 )
			int_to = TABLE_SIZE + 1; // fill in last 'extra' sample.

		for( ; table_index < int_to ; table_index++ )
		{
			m_lookup_table[table_index] = (float) (( (double)table_index ) * slope + c);
//			_RPT2(_CRT_WARN, "%3d : %f\n", table_index, m_lookup_table[table_index] );
		}

		from_x = to_x;
	}
}
