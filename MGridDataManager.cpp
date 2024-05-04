#include "MGridDataManager.h"
#include "MFileUtil.h"
#include "MTransform.h"
#include "MCollision.h"
#include "MMath.h"
#include "MBoxCollider.h"


#define GRID_META_FILE_NAME			MSTR("GridMetaData.dat")
#define GRID_DATA_FILE_PREFIX		MSTR("GridData")


//--------------------------------------------------------------------
// MGridData
//--------------------------------------------------------------------
void MGridData::Serialize(class MStream& inStream)
{
	inStream.Process(&GridIndex2D, sizeof(GridIndex2D));
	inStream.Process(&LeftTop, sizeof(LeftTop));
	inStream.Process(&RightBottom, sizeof(RightBottom));

	TileDataContainer.Serialize(inStream);
}


MTileData* MGridData::GetTileData(MINT32 inIndex)
{
	const MINT32 pos = sizeof(MTileData) * inIndex;

	const MINT32 size = TileDataContainer.GetSize();

	if (pos >= size)
	{
		int cc = 4;
		int dd = cc;
	}

	return (MTileData*)(TileDataContainer.GetPointer() + (sizeof(MTileData) * inIndex));
}


//--------------------------------------------------------------------
// MGridDataManager
//--------------------------------------------------------------------
MGridDataManager::MGridDataManager()
	:IsCreateGridDataFile(false)
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

MBOOL MGridDataManager::InitGridDataManager(const MString& inDataPath)
{
	GridDataPath = inDataPath;
	return MTRUE;
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
			MINT32 index = inValue / gridSideSize;
			if (inValue < 0) {
				index -= 1;
			}

			return index;
		};


	// 센터의 인덱스를 얻는다
	const MINT32 indexX = ConvertIndexFunc(inCenterPos.X);
	const MINT32 indexY = ConvertIndexFunc(inCenterPos.Y);
	
	return LoadGridDataByIndex(MIntPoint(indexX, indexY), inExtend);
}


MBOOL MGridDataManager::LoadGridDataByIndex(const MIntPoint& inCenterIndex, const MINT32 inExtend)
{
	MINT32 size = (inExtend * 2) + 1;
	LoadGridDataLogic(inCenterIndex.X - inExtend, inCenterIndex.Y - inExtend, size, size);
	return MTRUE;
}

MBOOL MGridDataManager::GetIndex2DByPosition(MIntPoint& inIndex2D, const MVector2& inPos)
{
	// 위치를 얻는다
	if (0 == LoadedGridCount) {
		return MFALSE;
	}

	MVector2 relativePos = inPos - LoaedGridLeftTopPos;
	if (relativePos.X < 0 || relativePos.Y < 0) {
		return MFALSE;
	}

	inIndex2D.X = (MINT32)(relativePos.X / GridMetaData.TileSize);
	inIndex2D.Y = (MINT32)(relativePos.Y / GridMetaData.TileSize);

	return MTRUE;
}


MTileData* MGridDataManager::GetTileDataByIndex2D(const MIntPoint& inIndex2D)
{
	MINT32 gridX = inIndex2D.X / GridMetaData.GridSideTileCount;
	MINT32 gridY = inIndex2D.Y / GridMetaData.GridSideTileCount;

	MINT32 gridIndex = (gridY * LoadedSize.Width) + gridX;

	if (LoadedGridDataContainer.size() <= gridIndex) {
		return nullptr;
	}

	MGridData* gridData = LoadedGridDataContainer[gridIndex];
	if (nullptr == gridData) {
		return nullptr;
	}

	MINT32 tileX = inIndex2D.X % GridMetaData.GridSideTileCount;
	MINT32 tileY = inIndex2D.Y % GridMetaData.GridSideTileCount;

	MINT32 tileIndex = (tileY * GridMetaData.GridSideTileCount) + tileX;
	
	return gridData->GetTileData(tileIndex);
}



MBOOL MGridDataManager::GetTileLeftTopPositionByIndex(MVector2& inPos, const MIntPoint& inIndex2D)
{
	inPos.X = LoaedGridLeftTopPos.X + (GridMetaData.TileSize * inIndex2D.X);
	inPos.Y = LoaedGridLeftTopPos.Y + (GridMetaData.TileSize * inIndex2D.Y);

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
	MMemoryI<1000> readMemory;

	// 최초 그리드 플래그
	// 이게 설정되면 lefttop을 그냥 설정
	LoadedGridCount = 0;

	// 루프를 돌면서 정보를 설정
	for (MINT32 x = 0; x < inWidth; ++x)
	{
		for (MINT32 y = 0; y < inHeight; ++y)
		{
			MIntPoint index2(inStartX + x, inStartY + y);
			
			// 대상 그리드 데이터를 얻는다
			MGridData* gridData = nullptr;
			{
				// 백업정보에 있는지 체크하고 있다면 그거 사용
				auto findIter = backupMap.find(index2);
				if (findIter == backupMap.end())
				{
					// 백업 정보에 없는경우 파일에서 로드
					MString fileName = GetGridDataFilePath(index2);

					// 대상 파일을 로드해보고 존재한다면 그거사용
					if (MTRUE == MFileUtil::LoadFile(readMemory, fileName))
					{
						// 풀에서 정보를 얻어오고 값을 카피
						gridData = GridDataPool.Pop();

						MMemoryReadStream readStream(readMemory);
						gridData->Serialize(readStream);
					}
					else
					{
						// 대상 파일이 없는경우
						if (MTRUE == IsCreateGridDataFile)
						{
							AddNewGridDataFile(fileName, index2);
							continue;
						}
					}
				}
				else
				{
					// 백업 정보에 있는경우
					gridData = findIter->second;
					backupMap.erase(findIter);
				}
			}

			// 넘어온 정보를 설정(nullptr 포함)
			LoadedGridDataContainer.push_back(gridData);
		
			// 그리드 정보가 있는경우 lefttop을 설정
			if(nullptr != gridData)
			{ 
				if (0 == LoadedGridCount)
				{
					LoaedGridLeftTopPos = gridData->LeftTop;
				}
				else
				{
					// 최초 설정이 아닌경우 비교
					LoaedGridLeftTopPos.X = MMath::Min(LoaedGridLeftTopPos.X, gridData->LeftTop.X);
					LoaedGridLeftTopPos.Y = MMath::Min(LoaedGridLeftTopPos.Y, gridData->LeftTop.Y);
				}

				++LoadedGridCount;
			}
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


MGridData* MGridDataManager::AddNewGridDataFile(const MString& inFileName, const MIntPoint& inIndex2)
{
	// 신규 파일을 생성	
	MGridData* gridData = GridDataPool.Pop();
	gridData->GridIndex2D = inIndex2;

	// 위치 정보
	MINT32 GridSideSize = GridMetaData.GetGridSideSize();
	gridData->LeftTop.Set(GridSideSize * inIndex2.X, GridSideSize * inIndex2.Y);
	gridData->RightBottom.Set(gridData->LeftTop.X + GridSideSize, gridData->LeftTop.Y + GridSideSize);

	// 타일 정보 할당
	gridData->TileDataContainer.Alloc(GridMetaData.GridSideTileCount * GridMetaData.GridSideTileCount * sizeof(MTileData));


	for (MINT32 x = 0; x < GridMetaData.GridSideTileCount; ++x)
	{
		for (MINT32 y = 0; y < GridMetaData.GridSideTileCount; ++y)
		{
			MINT32 idx = y * GridMetaData.GridSideTileCount + x;

			MTileData* tileData = gridData->GetTileData(idx);

			tileData->TileIndex2D.Set(x, y);
			tileData->IsObstacle = false;
		}
	}

	// 등록
	LoadedGridDataContainer.push_back(gridData);

	return gridData;
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
MGridDataEditManager::MGridDataEditManager()
	: MGridDataManager()
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
		
		const MINT32 tileCount = GridMetaData.GridSideTileCount * GridMetaData.GridSideTileCount;
		for ( MINT32 i=0;i<tileCount;++i)
		{
			MTileData* tileData = gridData->GetTileData(i);
			tileData->IsObstacle = false;

			MTransform tileTransform;
			tileTransform.Position = GetTileCenterPosition(gridData, tileData);
			
			MBoxCollider tileBoxCollider(tileTransform, MVector3(GridMetaData.TileSize, GridMetaData.TileSize, 100));

			for (MBoxCollider* collider : inColliderList)
			{
				// 충돌하는 박스
				if (true == MCollision::CheckOBB(tileBoxCollider, *collider))
				{
					tileData->IsObstacle = true;
					break;
				}
			}	
		}

		// 파일 저장
		SaveGridData(gridData);
	}
}

void MGridDataEditManager::SaveMetaData()
{
	MString metaFilePath = GetMetaFilePath();


}

void MGridDataEditManager::SaveGridData(MGridData* inGridData)
{
	// 데이터를 저장
	MMemoryI<1000> tempMemory;
	{
		MMemoryWriteStream writeStream(tempMemory);
		inGridData->Serialize(writeStream);
	}


	MString fileName = GetGridDataFilePath(inGridData->GridIndex2D);


	MFileUtil::SaveToFile(tempMemory.GetPointer(), tempMemory.GetSize(), fileName);
}

MVector3 MGridDataEditManager::GetTileCenterPosition(MGridData* inGridData, MTileData* inTileData)
{
	return MVector3(
		inGridData->LeftTop.X + (inTileData->TileIndex2D.X * GridMetaData.TileSize) + (GridMetaData.TileSize * 0.5f),
		inGridData->LeftTop.Y + (inTileData->TileIndex2D.Y * GridMetaData.TileSize) + (GridMetaData.TileSize * 0.5f),
		0);
}