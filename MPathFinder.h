#pragma once


#include "MPathFindData.h"
#include "MString.h"
#include "MVector.h"
#include <set>

#define PATH_FIND_INFINITY_DISTANCE MMath::MINT32_MAX


//----------------------------------------------------------------------
// ��� ã�⿡ ���Ǵ� ���
//----------------------------------------------------------------------
class MPathFindNode
{
public:
	void InitPathFindNode(const MIntPoint& inIndex2D, const MINT32 inDistanceH);

	// G�� ����
	void SetDistanceG(MINT32 inDistance)
	{
		DistanceG = inDistance;
		DistanceF = DistanceG + DistanceH;
	}

	void SetIsClose(MBOOL inFlag) {
		IsClose = inFlag;
	}

	// ���� ��� ����
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
	// �ε��� ����
	MIntPoint Index2D;

	// �Ÿ� ����
	MINT32 DistanceG = 0;		// ���� ��ġ���� �ش� ��� ������ �Ÿ�
	MINT32 DistanceH = 0;		// ��忡�� ���� ��ġ���� �Ÿ�
	MINT32 DistanceF = 0;		// G + H

	MPathFindNode* PrevNode = nullptr;

	MBOOL IsClose = MFALSE;
};



//----------------------------------------------------------------------
// ��� ã��(AStart)
//----------------------------------------------------------------------
class MPathFinder
{
public:
	MPathFinder();
	~MPathFinder();

public:
	// �ʱ�ȭ(��� ���� ��θ� ���ڷ� �޴´�)
	MBOOL InitPathFinder(const MString& inPathFindDataDir);


	// ��ġ�� �׸��� �����͸� �ε�
	MBOOL LoadGridDataByPosition(const MVector2& inCenterPos, const MINT32 inExtend);

	//-------------------------------------------------------------------
	// ��θ� ã�´�
	//-------------------------------------------------------------------
	MBOOL FindPath(std::vector<MVector2>& inList, const MVector2& inStartPos, const MVector2& inEndPos);


	class MGridDataManager* GetGridDataManager() {
		return GridDataManager;
	}
	
protected:
	// 
	MBOOL FindGridIndexPath(std::vector<MIntPoint>& inList, const MIntPoint& inStart, const MIntPoint& inEnd);


	// �ش� ��ġ�� ��带 ��´�
	MPathFindNode* GetNode(const MIntPoint& inIndex2D, const MIntPoint& inEndIndex2D);


	// ���� ��带 ���鼭 ������ ���鼭 F�� ���� ������ ã�´�
	MPathFindNode* GetMinDistanceFromOpenNodeSet();


	// �ֺ� ��带 ����
	void UpdateAroundNode(MPathFindNode* inBaseNode, const MIntPoint& inEndIndex2D);

	// �������� �Ÿ��� ��´�
	MINT32 GetGridDistanceByDirection(MINT32 inX, MINT32 inY) 
	{
		if (0 != inX && 0 != inY) {
			return 14;
		}
		return 10;
	}


	MBOOL CheckBlockLine(const MVector2& inStart, const MVector2& inEnd, MFLOAT inRadius);

protected:
	// �׸��� ������ �Ŵ���
	class MGridDataManager* GridDataManager = nullptr;

	// ��� ã�� ��� Ǯ
	MPool<MPathFindNode> NodePool;

	// ���ǰ� �ִ� ����
	std::map<MIntPoint, MPathFindNode*> UsedNodeMap;

	// ���� ��� �����̳�
	std::set<MPathFindNode*> OpenNodeSet;

};