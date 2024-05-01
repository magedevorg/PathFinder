#pragma once


#include "MPathFindData.h"
#include "MString.h"
#include "MVector.h"
#include <set>

#define PATH_FIND_INFINITY_DISTANCE MMath::MINT32_MAX


//----------------------------------------------------------------------
// 경로 찾기에 사용되는 노드
//----------------------------------------------------------------------
class MPathFindNode
{
public:
	void InitPathFindNode(const MIntPoint& inIndex2D, const MINT32 inDistanceH);

	// G를 설정
	void SetDistanceG(MINT32 inDistance)
	{
		DistanceG = inDistance;
		DistanceF = DistanceG + DistanceH;
	}

	void SetIsClose(MBOOL inFlag) {
		IsClose = inFlag;
	}

	// 이전 노드 설정
	void SetPrevNode(MPathFindNode* inPrevNode) {
		PrevNode = inPrevNode;
	}

	const MIntPoint& GetIndex2D() const {
		return Index2D;
	}


	MINT32 GetDistanceG() const {
		return DistanceG;
	}

	MINT32 GetDistanceF() const {
		return DistanceF;
	}

	MBOOL GetIsClose() const {
		return IsClose;
	}

	MPathFindNode* GetPrevNode() {
		return PrevNode;
	}

protected:
	// 인덱스 정보
	MIntPoint Index2D;

	// 거리 정보
	MINT32 DistanceG = 0;		// 시작 위치에서 해당 노드 까지의 거리
	MINT32 DistanceH = 0;		// 노드에서 종료 위치까지 거리
	MINT32 DistanceF = 0;		// G + H

	MPathFindNode* PrevNode = nullptr;

	MBOOL IsClose = MFALSE;
};



//----------------------------------------------------------------------
// 경로 찾기(AStart)
//----------------------------------------------------------------------
class MPathFinder
{
public:
	MPathFinder();
	~MPathFinder();

public:
	// 초기화(대상 파일 경로를 인자로 받는다)
	MBOOL InitPathFinder(const MString& inPathFindDataDir);


	// 위치로 그리드 데이터를 로드
	MBOOL LoadGridDataByPosition(const MVector2& inCenterPos, const MINT32 inExtend);

	//-------------------------------------------------------------------
	// 경로를 찾는다
	//-------------------------------------------------------------------
	MBOOL FindPath(std::vector<MVector2>& inList, const MVector2& inStartPos, const MVector2& inEndPos);


	class MGridDataManager* GetGridDataManager() {
		return GridDataManager;
	}
	
protected:
	// 
	MBOOL FindGridIndexPath(std::vector<MIntPoint>& inList, const MIntPoint& inStart, const MIntPoint& inEnd);


	// 해당 위치의 노드를 얻는다
	MPathFindNode* GetNode(const MIntPoint& inIndex2D, const MIntPoint& inEndIndex2D);


	// 열린 노드를 돌면서 루프를 돌면서 F가 가장 작은걸 찾는다
	MPathFindNode* GetMinDistanceFromOpenNodeSet();


	// 주변 노드를 갱신
	void UpdateAroundNode(MPathFindNode* inBaseNode, const MIntPoint& inEndIndex2D);

	// 방향으로 거리를 얻는다
	MINT32 GetGridDistanceByDirection(MINT32 inX, MINT32 inY) 
	{
		if (0 != inX && 0 != inY) {
			return 14;
		}
		return 10;
	}


	MBOOL CheckBlockLine(const MVector2& inStart, const MVector2& inEnd, MFLOAT inRadius);

protected:
	// 그리드 데이터 매니저
	class MGridDataManager* GridDataManager = nullptr;

	// 경로 찾기 노드 풀
	MPool<MPathFindNode> NodePool;

	// 사용되고 있는 노드맵
	std::map<MIntPoint, MPathFindNode*> UsedNodeMap;

	// 열린 노드 컨테이너
	std::set<MPathFindNode*> OpenNodeSet;

};