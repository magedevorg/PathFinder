#pragma once


#include "MPathFindData.h"
#include "MString.h"
#include "MVector.h"



class MPathFinder
{
public:
	// 초기화(대상 파일 경로를 인자로 받는다)
	void InitPathFinder(const MString& inPath);
	

	//-------------------------------------------------------------------
	// 경로를 찾는다
	//-------------------------------------------------------------------
	// 인덱스로 경로를 찾는다
	MBOOL FindPath(std::vector<MIntPoint>& inList, const MIntPoint& inStart, const MIntPoint& inEnd);

	

protected:
	


protected:
	// 메타 데이터
	MPathFindMetaData MetaData;

	// 그리드 정보 컨테이너
	std::vector<MGridData> GridDataContainer;
};