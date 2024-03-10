#pragma once

#include "MPrerequisites.h"
#include "MType.h"
#include "MVector.h"


class MTileData
{
public:
	// ��ġ ����
	MIntPoint TileIndex2D;

	// ��ֹ� ����
	bool IsObstacle = false;
};



class MGridData
{
public:
	// 인덱스
	MIntPoint GridIndex2D;

	// 위치 정보
	MVector2 LeftTop;
	MVector2 RightBottom;

	std::vector<MTileData> TileDataContainer;
};



class MPathFindMetaData
{
public:
	// 왼쪽 상단 위치(시작위치)
	MVector2 LeftTopPos;

	// 맵 사이즈
	MIntPoint MapSize;

	//--------------------------------------------------------
	// 그리드 정보
	//--------------------------------------------------------
	MINT32 GridSize;		// 그리드 사이즈
	MIntSize GridCount;		// 그리드 카운트

	//--------------------------------------------------------
	// 타일 정보
	//--------------------------------------------------------
	MINT32 TileSize;		// 타일 사이즈
	MINT32 LineTileCount;	// 한라인의 타일 개수

};


class MPathFindData
{
public:
	// 메타 데이터
	MPathFindMetaData MetaData;

	// 컨테이너
	std::vector<MGridData> GridDataContainer;
};