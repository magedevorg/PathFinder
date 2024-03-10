#pragma once


#include "MPathFindData.h"
#include "MString.h"
#include "MVector.h"



class MPathFinder
{
public:
	// �ʱ�ȭ(��� ���� ��θ� ���ڷ� �޴´�)
	void InitPathFinder(const MString& inPathFindDataDir);
	
	//-------------------------------------------------------------------
	// ��θ� ã�´�
	//-------------------------------------------------------------------
	// �ε����� ��θ� ã�´�
	MBOOL FindPath(std::vector<MIntPoint>& inList, const MIntPoint& inStart, const MIntPoint& inEnd);

protected:
	


protected:
	// �н� ã�� ������ ���͸�
	MString PathFindDataDir;

	// ��Ÿ ������
	MPathFindMetaData MetaData;

	// �׸��� ���� �����̳�
	std::vector<MGridData> GridDataContainer;
};