#include "MPathFinder.h"
#include "MGridDataManager.h"

#include "MBoxCollider.h"




//---------------------------------------------------------------------
// MPathFindNode
//---------------------------------------------------------------------
void MPathFindNode::InitPathFindNode(const MIntPoint& inIndex2D, const MINT32 inDistanceH)
{
	Index2D = inIndex2D;

	// 이전 경로 데이터 초기화
	PrevNode = nullptr;

	// 경로 정보 초기화
	DistanceH = inDistanceH;
	SetDistanceG(PATH_FIND_INFINITY_DISTANCE);


	IsClose = MFALSE;
}



//---------------------------------------------------------------------
// MPathFinder
//---------------------------------------------------------------------
MPathFinder::MPathFinder()
{
	// 그리드 데이터 매니저 생성
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
	// 그리드 데이터 매니저 초기화
	if (MFALSE == GridDataManager->InitGridDataManager(inDataPath)) {
		return MFALSE;
	}

	// 메타데이터가 존재하는지 체크
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

	// 시작/종료 위치의 인덱스를 구한다
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
	
	// 동일한 인덱스인 경우 그냥 결과 위치로 이동
	if (startIndex == endIndex) 
	{
		inList.push_back(inEndPos);
		return MTRUE;
	}

	// 인덱스 리스트를 구한다
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
		// 인덱스로 그리드의 위치를 얻는다
		positionList[i].X = leftTopPos.X + (metaData->TileSize * index2DList[i].X) + halfTileSize;
		positionList[i].Y = leftTopPos.Y + (metaData->TileSize * index2DList[i].Y) + halfTileSize;
	}

	// 마지막 위치는 종료 위치
	if (0 < count) {
		positionList[count - 1] = inEndPos;
	}

	
	// OBB 체크
	// 마지막 인덱스
	const MINT32 lastIndex = positionList.size() - 1;

	MINT32 checkIndex = 0;

	// 시작점 추가
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
	// 시작타일과 종료 타일을 얻는다
	const MTileData* startTile = GridDataManager->GetTileDataByIndex2D(inStart);
	if (nullptr == startTile) {
		return MFALSE;
	}

	const MTileData* endTile = GridDataManager->GetTileDataByIndex2D(inEnd);
	if (nullptr == endTile) {
		return MFALSE;
	}

	// 시작 노드 설정
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
			// 더이상 열린 노드가 없다
			break;
		}

		if (checkNode->GetIndex2D() == inEnd)
		{
			// 끝에 종료 위치까지 도달
			break;
		}

		// 열린셋에서제거
		OpenNodeSet.erase(checkNode);

		// 닫힘 처리
		checkNode->SetIsClose(MTRUE);

		// 정보를 갱신
		UpdateAroundNode(checkNode, inEnd);
	}

	// 결과 위치에서 역추적한다
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

		// 역순으로 변경
		std::reverse(inList.begin(), inList.end());
	}
	
	// 반납
	{
		// 사용했던 노드는 풀에 반납
		for (auto pair : UsedNodeMap) {
			NodePool.Push(pair.second);
		}

		UsedNodeMap.clear();

		// 데이터 셋 제거
		OpenNodeSet.clear();
	}

	return MTRUE;
}



MPathFindNode* MPathFinder::GetNode(const MIntPoint& inIndex2D, const MIntPoint& inEndIndex2D)
{
	// 현재 사용되는 노드인 경우 그거를 리턴
	auto findIter = UsedNodeMap.find(inIndex2D);
	if (UsedNodeMap.end() != findIter) {
		return findIter->second;
	}

	// 없다면 풀에서 가져옴
	MPathFindNode* node = NodePool.Pop();
	
	// 사용맵에 추가
	UsedNodeMap.emplace(std::make_pair(inIndex2D, node));

	// 해당 노드에서 결과 위치까지의 거리를 얻는다(맨허튼 거리 측정)
	const MINT32 distanceH = (MMath::ABS(inEndIndex2D.X - inIndex2D.X) * 10) + (MMath::ABS(inEndIndex2D.Y - inIndex2D.Y) * 10);

	// 초기화
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
	// 인덱스
	const MIntPoint baseIndex2D = inBaseNode->GetIndex2D();

	// 해당 노드의 거리정보
	const MINT32 baseDistanceG = inBaseNode->GetDistanceG();

	// 주변 노드 루프
	for (MINT32 x = -1; x <= 1; ++x)
	{
		for (MINT32 y = -1; y <= 1; ++y)
		{
			// 자신의 노드인경우는 넘어간다
			if (0 == x && 0 == y) {
				continue;
			}

			// 대각선을 처리하지 않는다
			if (0 != x && 0 != y) {
				continue;
			}


			// 대상 인덱스 정보
			const MIntPoint targetIndex2D = baseIndex2D + MIntPoint(x, y);

			// 대상 노드를 얻는다
			MPathFindNode* targetNode = GetNode(targetIndex2D, inEndIndex2D);

			// 닫힌노드인경우 넘어간다
			if (MTRUE == targetNode->GetIsClose()) {
				continue;
			}

			// 체크할 타일
			const MTileData* checkTile = GridDataManager->GetTileDataByIndex2D(targetIndex2D);
			if (nullptr == checkTile) {
				continue;
			}

			// 대상 타일이 막힌경우 넘어간다
			if (MTRUE == checkTile->IsObstacle) {
				continue;
			}

			// 대상 노드는 열린 노드에 추가
			OpenNodeSet.insert(targetNode);

			//----------------------------------------------------------------
			// 거리를 체크해서 가까운경우 이동 정보를 갱신
			//----------------------------------------------------------------

			// 거리를 얻는다
			const MINT32 gridDistance = GetGridDistanceByDirection(x, y);

			// 대상의 시작 위치에서 거리
			const MINT32 targetDistanceG = targetNode->GetDistanceG();

			// 기본노드를 거쳐서 대상 노드까지의 거리를 구한다
			const MINT32 newTargetDistanceG = baseDistanceG + gridDistance;

			// 기존에 설정되어있던 거리보다 적다면 이동 정보를 갱신해준다
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

			// 방향 벡터
			MVector3 dir3D = end3D - start3D;
			dir3D.Normalize();

			// 왼쪽 벡터
			MVector3 leftVec3D = MVector3::CrossProduct(dir3D, MVector3(0, 0, 1));
			leftVec.Set(leftVec3D.X, leftVec3D.Y);
			leftVec.Normalize();
			leftVec *= inRadius;
		}

		// 각 위치 설정
		rect1.Set(inEnd + leftVec, inEnd - leftVec, inStart + leftVec, inStart - leftVec);
	}



	MIntPoint startIndex2D;
	GridDataManager->GetIndex2DByPosition(startIndex2D, inStart);

	MIntPoint endIndex2D;
	GridDataManager->GetIndex2DByPosition(endIndex2D, inEnd);
	
	// 시작 ~ 종료 블록 루프
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
				// 충돌한다면 대상 그리드가 장애물인지 체크
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