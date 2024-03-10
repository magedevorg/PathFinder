#include "MPathFinder.h"


void MPathFinder::InitPathFinder(const MString& inPathFindDataDir)
{
	// 경로 설정
	PathFindDataDir = inPathFindDataDir;
}


MBOOL MPathFinder::FindPath(std::vector<MIntPoint>& inList, const MIntPoint& inStart, const MIntPoint& inEnd)
{
	return MFALSE;
}