#pragma once
#include <vector>
#include <string>
#include "cagd.h"
using std::vector;

enum ParseState
{
	ParseStateNone,
	ParseStateFoundNumber,
	ParseStateKnots,
	ParseStatePoints,
	ParseStateDone
};


enum SplineType
{
	SplineTypeUnknown,
	SplineTypeBezier,
	SplineTypeBspline
};
class BezierParser
{
public:
	BezierParser(void);
	~BezierParser(void);
	SplineType Type() const;
	void Clear();
	
	bool ParseFile(const std::string& filename);
	bool ParseLine(const std::string& line);

	vector<CCagdPoint> m_pts;
	vector<double> m_knots;

	int m_expectedPtCount;
	int m_expectedKnotCount;

	ParseState m_state;
	SplineType m_type;
};
