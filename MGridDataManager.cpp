#include "MGridDataManager.h"
#include "MFileUtil.h"
#include "MTransform.h"
#include "MCollision.h"


#define GRID_META_FILE_NAME			MSTR("GridMetaData.dat")
#define GRID_DATA_FILE_PREFIX		MSTR("GridData")


//--------------------------------------------------------------------
// MGridDataManager
//--------------------------------------------------------------------
MGridDataManager::MGridDataManager(const MString& inDataPath)
	: GridDataPath(inDataPath)
	, IsCreateGridDataFile(false)
{
	GridDataPool.InitPool(MFALSE, 9, MTRUE, []()->MGridData*
		{
			return new MGridData();
		});
}

MGridDataManager::~MGridDataManager()
{
	// 모두 풀로 반납
	const MINT32 count = LoadedGridDataContainer.size();
	for (MINT32 i = 0; i < count; ++i) {
		GridDataPool.Push(LoadedGridDataContainer[i]);
	}
}


MBOOL MGridDataManager::LoadMetaData()
{
	// 해당 데이터 경로에서 메타 데이터를 얻어온다
	MString metaFilePath = GetMetaFilePath();

	MMemory tempMemory;
	if (MFALSE == MFileUtil::LoadFile(tempMemory, metaFilePath)) {
		return MFALSE;
	}

	// 사이즈 체크
	const MSIZE loadSize = tempMemory.GetSize();
	const MSIZE metaSize = sizeof(MGridMetaData);

	// 사이즈가 다른경우 에러
	if (loadSize != metaSize) {
		MFALSE;
	}

	// 정보를 카피
	::memcpy(&GridMetaData, tempMemory.GetPointer(), metaSize);

	// 성공
	return MTRUE;
}


MBOOL MGridDataManager::LoadGridDataByPosition(const MVector2& inCenterPos, const MINT32 inExtend)
{
	MINT32 gridSideSize = GridMetaData.GetGridSideSize();

	// 위치 -> 인덱스 변경 람다
	auto ConvertIndexFunc = [gridSideSize](MFLOAT inValue)->MINT32
		{
			int32 index = inValue / gridSideSize;
			if (inValue < 0) {
				index -= 1;
			}

			return index;
		};


	// 센터의 인덱스를 얻는다
	const int32 indexX = ConvertIndexFunc(inCenterPos.X);
	const int32 indexY = ConvertIndexFunc(inCenterPos.Y);
	
	return LoadGridDataByIndex(MIntPoint(indexX, indexY), inExtend);
}


MBOOL MGridDataManager::LoadGridDataByIndex(const MIntPoint& inCenterIndex, const MINT32 inExtend)
{
	MINT32 size = (inExtend * 2) + 1;
	LoadGridDataLogic(inCenterIndex.X - inExtend, inCenterIndex.Y - inExtend, size, size);
	return MTRUE;
}


void MGridDataManager::LoadGridDataLogic(MINT32 inStartX, MINT32 inStartY, MINT32 inWidth, MINT32 inHeight)
{
	// 이전에 로드된 정보와 동일한지 체크
	if (LoadedStartIndex.X == inStartX && LoadedStartIndex.Y == inStartY &&
		LoadedSize.Width == inWidth && LoadedSize.Height == inHeight)
	{
		return;
	}

	// 기존 정보를 백업
	std::map<MIntPoint, MGridData*> backupMap;
	{
		MINT32 count = LoadedGridDataContainer.size();
		for (MINT32 i = 0; i < count; ++i)
		{
			if (MGridData* gridData = LoadedGridDataContainer[i]) {
				backupMap.emplace(gridData->GridIndex2D, gridData);
			}
		}
	}

	// 기존 정보는 클리어
	LoadedGridDataContainer.clear();


	// 그리드 데이터용 메모리
	MMemoryI<sizeof(MGridData)> readMemory;

	// 루프를 돌면서 정보를 설정
	for (MINT32 x = 0; x < inWidth; ++x)
	{
		for (MINT32 y = 0; y < inHeight; ++y)
		{
			MIntPoint index2(inStartX + x, inStartY + y);
			
			
			// 백업정보에 있는지 체크하고 있다면 그거 사용
			auto findIter = backupMap.find(index2);
			if (findIter != backupMap.end())
			{
				LoadedGridDataContainer.push_back(findIter->second);
				backupMap.erase(findIter);
				continue;
			}


			// 대상이 없는경우 파일에서 불러온다
			MString fileName = GetGridDataFilePath(index2);

			// 이부분 벡터를 덮어 씌우기 때문에 위험한 코드 
			// 우선 그냥 처리해분다
			/* 
			// 대상 파일을 로드해보고 존재한다면 그거사용
			if ( MTRUE == MFileUtil::LoadFile(readMemory, fileName) )
			{
				// 풀에서 정보를 얻어오고 값을 카피
				MGridData* gridData = GridDataPool.Pop();
				readMemory.Read(gridData, 0, sizeof(MGridData));

				LoadedGridDataContainer.push_back(gridData);
				continue;
			}
			*/


			if (MTRUE == IsCreateGridDataFile)
			{
				// 신규 파일을 생성	
				MGridData* gridData = GridDataPool.Pop();
				gridData->GridIndex2D = index2;

				// 위치 정보
				MINT32 GridSideSize = GridMetaData.GetGridSideSize();
				gridData->LeftTop.X = GridSideSize * index2.X;
				gridData->LeftTop.Y = GridSideSize * index2.Y;

				gridData->TileDataContainer.clear();
				gridData->TileDataContainer.resize(GridMetaData.GridSideTileCount * GridMetaData.GridSideTileCount);
				for (int32 ty = 0; ty < GridMetaData.GridSideTileCount; ++ty)
				{
					for (int32 tx = 0; tx < GridMetaData.GridSideTileCount; ++tx)
					{
						int32 idx = ty * GridMetaData.GridSideTileCount + tx;
						gridData->TileDataContainer[idx].TileIndex2D.Set(tx, ty);
						gridData->TileDataContainer[idx].IsObstacle = false;
					}
				}

				LoadedGridDataContainer.push_back(gridData);

				MFileUtil::SaveToFile(gridData, sizeof(MGridData), fileName);
				continue;
			}
			
			// 이것도 저것도 아니면 nullptr 추가
			LoadedGridDataContainer.push_back(nullptr);
		}
	}


	// 정보 설정
	LoadedStartIndex.Set(inStartX, inStartY);
	LoadedSize.Set(inWidth, inHeight);


	// 미사용 그리드 정보는 풀에 반납
	for (auto& pair : backupMap) {
		GridDataPool.Push(pair.second);
	}
}

MString MGridDataManager::GetMetaFilePath() const
{
	return GridDataPath + GRID_META_FILE_NAME;
}

MString MGridDataManager::GetGridDataFilePath(MINT32 inX, MINT32 inY) const
{
	return MString::Format(MSTR("%s%s_%d_%d.grid"), GridDataPath.GetStr(), GRID_DATA_FILE_PREFIX, inX, inY);
}

MString MGridDataManager::GetGridDataFilePath(const MIntPoint& inIndex2) const
{
	return GetGridDataFilePath(inIndex2.X, inIndex2.Y);
}

//--------------------------------------------------------------------
// MGridDataEditManager
//--------------------------------------------------------------------
MGridDataEditManager::MGridDataEditManager(const MString& inDataPath)
	: MGridDataManager(inDataPath)
{
	IsCreateGridDataFile = true;
}

MBOOL MGridDataEditManager::ResetMetaData(const MGridMetaData& inMetaData)
{
	// 메타 파일 저장
	if (MFALSE == MFileUtil::SaveToFile(&inMetaData, sizeof(inMetaData), GetMetaFilePath())) {
		return MFALSE;
	}

	// 메모리에서 정리
	LoadedGridDataContainer.clear();

	// 메타 데이터 재설정
	GridMetaData = inMetaData;

	return MTRUE;
}


void MGridDataEditManager::UpdateLoadedGridData(std::vector<class MBoxCollider*>& inColliderList)
{
	// 그리드 데이터 루프
	for (MGridData* gridData : LoadedGridDataContainer)
	{
		if (nullptr == gridData) {
			continue;
		}

		for (MTileData& tileData : gridData->TileDataContainer)
		{
			tileData.IsObstacle = false;

			MTransform tileTransform;
			tileTransform.Position = GetTileCenterPosition(gridData, &tileData);
			
			MBoxCollider tileBoxCollider(tileTransform, MVector3(GridMetaData.TileSize, GridMetaData.TileSize, 100));

			for (MBoxCollider* collider : inColliderList)
			{
				// 충돌하는 박스
				if (true == MCollision::CheckOBB(tileBoxCollider, *collider))
				{
					tileData.IsObstacle = true;
					break;
				}
			}	
		}
	}
}

void MGridDataEditManager::SaveMetaData()
{
	MString metaFilePath = GetMetaFilePath();


}

void MGridDataEditManager::SaveGridData()
{

}

MVector3 MGridDataEditManager::GetTileCenterPosition(MGridData* inGridData, MTileData* inTileData)
{
	return MVector3(
		inGridData->LeftTop.X + (inTileData->TileIndex2D.X * GridMetaData.TileSize) + (GridMetaData.TileSize * 0.5f),
		inGridData->LeftTop.Y + (inTileData->TileIndex2D.Y * GridMetaData.TileSize) + (GridMetaData.TileSize * 0.5f),
		0);
}