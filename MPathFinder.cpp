#include "MPathFinder.h"
#include "MGridDataManager.h"

#include "MBoxCollider.h"




//---------------------------------------------------------------------
// MPathFindNode
//---------------------------------------------------------------------
void MPathFindNode::InitPathFindNode(const MIntPoint& inIndex2D, const MINT32 inDistanceH)
{
	Index2D = inIndex2D;

	// ���� ��� ������ �ʱ�ȭ
	PrevNode = nullptr;

	// ��� ���� �ʱ�ȭ
	DistanceH = inDistanceH;
	SetDistanceG(PATH_FIND_INFINITY_DISTANCE);


	IsClose = MFALSE;
}



//---------------------------------------------------------------------
// MPathFinder
//---------------------------------------------------------------------
MPathFinder::MPathFinder()
{
	// �׸��� ������ �Ŵ��� ����
	GridDataManager = new MGridDataManager();

	NodePool.InitPool(false, 10, MTRUE, []()->MPathFindNode*
		{
			return new MPathFindNode();
		});
}


MPathFinder::~MPathFinder()
{
	if (nullptr != GridDataManager) 
	{
		delete GridDataManager;
		GridDataManager = nullptr;
	}
}



MBOOL MPathFinder::InitPathFinder(const MString& inDataPath)
{
	// �׸��� ������ �Ŵ��� �ʱ�ȭ
	if (MFALSE == GridDataManager->InitGridDataManager(inDataPath)) {
		return MFALSE;
	}

	// ��Ÿ�����Ͱ� �����ϴ��� üũ
	if (MFALSE == GridDataManager->LoadMetaData()) {
		return MFALSE;
	}

	return MTRUE;
}


MBOOL MPathFinder::LoadGridDataByPosition(const MVector2& inCenterPos, const MINT32 inExtend)
{
	if (MFALSE == GridDataManager->LoadGridDataByPosition(inCenterPos, inExtend)) {
		return MFALSE;
	}

	// 


	return MTRUE;
}

MBOOL MPathFinder::FindPath(std::vector<MVector2>& inList, const MVector2& inStartPos, const MVector2& inEndPos)
{
	inList.clear();

	// ����/���� ��ġ�� �ε����� ���Ѵ�
	MIntPoint startIndex;
	MIntPoint endIndex;
	{
		if (MFALSE == GridDataManager->GetIndex2DByPosition(startIndex, inStartPos)) {
			return MFALSE;
		}

		if (MFALSE == GridDataManager->GetIndex2DByPosition(endIndex, inEndPos)) {
			return MFALSE;
		}
	}
	
	// ������ �ε����� ��� �׳� ��� ��ġ�� �̵�
	if (startIndex == endIndex) 
	{
		inList.push_back(inEndPos);
		return MTRUE;
	}

	// �ε��� ����Ʈ�� ���Ѵ�
	std::vector<MIntPoint> index2DList;
	if (MFALSE == FindGridIndexPath(index2DList, startIndex, endIndex)) {
		return MFALSE;
	}

	const int32 count = index2DList.size();
	std::vector<MVector2> positionList;
	positionList.resize(count);


	MGridMetaData* metaData = GridDataManager->GetGridMetaData();

	MVector2 leftTopPos = GridDataManager->GetLoaedGridLeftTopPos();

	MFLOAT halfTileSize = metaData->TileSize * 0.5f;

	for (MINT32 i = 0; i < count; ++i) 
	{
		// �ε����� �׸����� ��ġ�� ��´�
		positionList[i].X = leftTopPos.X + (metaData->TileSize * index2DList[i].X) + halfTileSize;
		positionList[i].Y = leftTopPos.Y + (metaData->TileSize * index2DList[i].Y) + halfTileSize;
	}

	// ������ ��ġ�� ���� ��ġ
	if (0 < count) {
		positionList[count - 1] = inEndPos;
	}

	
	// OBB üũ
	// ������ �ε���
	const MINT32 lastIndex = positionList.size() - 1;

	MINT32 checkIndex = 0;

	// ������ �߰�
	inList.push_back(inStartPos);
	while (lastIndex != checkIndex)
	{
		MBOOL isCollision = MTRUE;

		for (MINT32 i = lastIndex; checkIndex < i; --i)
		{
			if (MFALSE == CheckBlockLine(positionList[checkIndex], positionList[i], 10))
			{
				checkIndex = i;
				inList.push_back(positionList[i]);
				isCollision = MFALSE;
				break;
			}
		}

		if (MTRUE == isCollision)
		{
			++checkIndex;
			inList.push_back(positionList[checkIndex]);
		}
	}

	return MTRUE;
}



MBOOL MPathFinder::FindGridIndexPath(std::vector<MIntPoint>& inList, const MIntPoint& inStart, const MIntPoint& inEnd)
{
	// ����Ÿ�ϰ� ���� Ÿ���� ��´�
	const MTileData* startTile = GridDataManager->GetTileDataByIndex2D(inStart);
	if (nullptr == startTile) {
		return MFALSE;
	}

	const MTileData* endTile = GridDataManager->GetTileDataByIndex2D(inEnd);
	if (nullptr == endTile) {
		return MFALSE;
	}

	// ���� ��� ����
	{
		MPathFindNode* startNode = GetNode(inStart, inEnd);
		startNode->SetDistanceG(0);
		OpenNodeSet.insert(startNode);
	}

	// 
	while (MTRUE)
	{
		MPathFindNode* checkNode = GetMinDistanceFromOpenNodeSet();
		if (nullptr == checkNode)
		{
			// ���̻� ���� ��尡 ����
			break;
		}

		if (checkNode->GetIndex2D() == inEnd)
		{
			// ���� ���� ��ġ���� ����
			break;
		}

		// �����¿�������
		OpenNodeSet.erase(checkNode);

		// ���� ó��
		checkNode->SetIsClose(MTRUE);

		// ������ ����
		UpdateAroundNode(checkNode, inEnd);
	}

	// ��� ��ġ���� �������Ѵ�
	{
		auto findIter = UsedNodeMap.find(inEnd);
		if (UsedNodeMap.end() != findIter)
		{
			MPathFindNode* currentNode = findIter->second;
			while (nullptr != currentNode)
			{
				inList.push_back(currentNode->GetIndex2D());
				currentNode = currentNode->GetPrevNode();
			}
		}

		// �������� ����
		std::reverse(inList.begin(), inList.end());
	}
	
	// �ݳ�
	{
		// ����ߴ� ���� Ǯ�� �ݳ�
		for (auto pair : UsedNodeMap) {
			NodePool.Push(pair.second);
		}

		UsedNodeMap.clear();

		// ������ �� ����
		OpenNodeSet.clear();
	}

	return MTRUE;
}



MPathFindNode* MPathFinder::GetNode(const MIntPoint& inIndex2D, const MIntPoint& inEndIndex2D)
{
	// ���� ���Ǵ� ����� ��� �װŸ� ����
	auto findIter = UsedNodeMap.find(inIndex2D);
	if (UsedNodeMap.end() != findIter) {
		return findIter->second;
	}

	// ���ٸ� Ǯ���� ������
	MPathFindNode* node = NodePool.Pop();
	
	// ���ʿ� �߰�
	UsedNodeMap.emplace(std::make_pair(inIndex2D, node));

	// �ش� ��忡�� ��� ��ġ������ �Ÿ��� ��´�(����ư �Ÿ� ����)
	const MINT32 distanceH = (MMath::ABS(inEndIndex2D.X - inIndex2D.X) * 10) + (MMath::ABS(inEndIndex2D.Y - inIndex2D.Y) * 10);

	// �ʱ�ȭ
	node->InitPathFindNode(inIndex2D, distanceH);

	return node;
}



MPathFindNode* MPathFinder::GetMinDistanceFromOpenNodeSet()
{
	MINT32 minValue = PATH_FIND_INFINITY_DISTANCE;
	MPathFindNode* nextNode = nullptr;

	for (auto targetNode : OpenNodeSet)
	{
		const MINT32 distanceF = targetNode->GetDistanceF();

		if (distanceF < minValue)
		{
			minValue = distanceF;
			nextNode = targetNode;
		}
	}

	return nextNode;
}



void MPathFinder::UpdateAroundNode(MPathFindNode* inBaseNode, const MIntPoint& inEndIndex2D)
{
	// �ε���
	const MIntPoint baseIndex2D = inBaseNode->GetIndex2D();

	// �ش� ����� �Ÿ�����
	const MINT32 baseDistanceG = inBaseNode->GetDistanceG();

	// �ֺ� ��� ����
	for (MINT32 x = -1; x <= 1; ++x)
	{
		for (MINT32 y = -1; y <= 1; ++y)
		{
			// �ڽ��� ����ΰ��� �Ѿ��
			if (0 == x && 0 == y) {
				continue;
			}

			// �밢���� ó������ �ʴ´�
			if (0 != x && 0 != y) {
				continue;
			}


			// ��� �ε��� ����
			const MIntPoint targetIndex2D = baseIndex2D + MIntPoint(x, y);

			// ��� ��带 ��´�
			MPathFindNode* targetNode = GetNode(targetIndex2D, inEndIndex2D);

			// ��������ΰ�� �Ѿ��
			if (MTRUE == targetNode->GetIsClose()) {
				continue;
			}

			// üũ�� Ÿ��
			const MTileData* checkTile = GridDataManager->GetTileDataByIndex2D(targetIndex2D);
			if (nullptr == checkTile) {
				continue;
			}

			// ��� Ÿ���� ������� �Ѿ��
			if (MTRUE == checkTile->IsObstacle) {
				continue;
			}

			// ��� ���� ���� ��忡 �߰�
			OpenNodeSet.insert(targetNode);

			//----------------------------------------------------------------
			// �Ÿ��� üũ�ؼ� ������� �̵� ������ ����
			//----------------------------------------------------------------

			// �Ÿ��� ��´�
			const MINT32 gridDistance = GetGridDistanceByDirection(x, y);

			// ����� ���� ��ġ���� �Ÿ�
			const MINT32 targetDistanceG = targetNode->GetDistanceG();

			// �⺻��带 ���ļ� ��� �������� �Ÿ��� ���Ѵ�
			const MINT32 newTargetDistanceG = baseDistanceG + gridDistance;

			// ������ �����Ǿ��ִ� �Ÿ����� ���ٸ� �̵� ������ �������ش�
			if (newTargetDistanceG < targetDistanceG)
			{
				targetNode->SetDistanceG(newTargetDistanceG);
				targetNode->SetPrevNode(inBaseNode);
			}
		}
	}
}




MBOOL MPathFinder::CheckBlockLine(const MVector2& inStart, const MVector2& inEnd, MFLOAT inRadius)
{
	MRect rect1;
	{
		MVector2 leftVec;
		{
			MVector3 start3D(inStart.X, inStart.Y, 0);
			MVector3 end3D(inEnd.X, inEnd.Y, 0);

			// ���� ����
			MVector3 dir3D = end3D - start3D;
			dir3D.Normalize();

			// ���� ����
			MVector3 leftVec3D = MVector3::CrossProduct(dir3D, MVector3(0, 0, 1));
			leftVec.Set(leftVec3D.X, leftVec3D.Y);
			leftVec.Normalize();
			leftVec *= inRadius;
		}

		// �� ��ġ ����
		rect1.Set(inEnd + leftVec, inEnd - leftVec, inStart + leftVec, inStart - leftVec);
	}



	MIntPoint startIndex2D;
	GridDataManager->GetIndex2DByPosition(startIndex2D, inStart);

	MIntPoint endIndex2D;
	GridDataManager->GetIndex2DByPosition(endIndex2D, inEnd);
	
	// ���� ~ ���� ��� ����
	MINT32 startX = startIndex2D.X;
	MINT32 endX = endIndex2D.X;
	if (endX < startX) {
		MMath::Swap(startX, endX);
	}

	MINT32 startY = startIndex2D.Y;
	MINT32 endY = endIndex2D.Y;
	if (endY < startY) {
		MMath::Swap(startY, endY);
	}


	MFLOAT tileSize = GridDataManager->GetGridMetaData()->TileSize;

	for (MINT32 x = startX; x <= endX; ++x)
	{
		for (MINT32 y = startY; y <= endY; ++y)
		{
			MIntPoint targetPos(x, y);

			MVector2 leftTop;
			GridDataManager->GetTileLeftTopPositionByIndex(leftTop, targetPos);

			MRect rect2;
			rect2.Set(leftTop, leftTop + MVector2(tileSize, 0), leftTop + MVector2(0, tileSize), leftTop + MVector2(tileSize, tileSize));

			if (MTRUE == MCollision::CheckOBB(rect1, rect2))
			{
				// �浹�Ѵٸ� ��� �׸��尡 ��ֹ����� üũ
				if (MTileData* tileData = GridDataManager->GetTileDataByIndex2D(targetPos))
				{
					if (MTRUE == tileData->IsObstacle) {
						return MTRUE;
					}
				}
			}
		}
	}

	return MFALSE;
}