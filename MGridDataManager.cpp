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
	// ��� Ǯ�� �ݳ�
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
	// �ش� ������ ��ο��� ��Ÿ �����͸� ���´�
	MString metaFilePath = GetMetaFilePath();

	MMemory tempMemory;
	if (MFALSE == MFileUtil::LoadFile(tempMemory, metaFilePath)) {
		return MFALSE;
	}

	// ������ üũ
	const MSIZE loadSize = tempMemory.GetSize();
	const MSIZE metaSize = sizeof(MGridMetaData);

	// ����� �ٸ���� ����
	if (loadSize != metaSize) {
		MFALSE;
	}

	// ������ ī��
	::memcpy(&GridMetaData, tempMemory.GetPointer(), metaSize);

	// ����
	return MTRUE;
}


MBOOL MGridDataManager::LoadGridDataByPosition(const MVector2& inCenterPos, const MINT32 inExtend)
{
	MINT32 gridSideSize = GridMetaData.GetGridSideSize();

	// ��ġ -> �ε��� ���� ����
	auto ConvertIndexFunc = [gridSideSize](MFLOAT inValue)->MINT32
		{
			MINT32 index = inValue / gridSideSize;
			if (inValue < 0) {
				index -= 1;
			}

			return index;
		};


	// ������ �ε����� ��´�
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
	// ��ġ�� ��´�
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
	// ������ �ε�� ������ �������� üũ
	if (LoadedStartIndex.X == inStartX && LoadedStartIndex.Y == inStartY &&
		LoadedSize.Width == inWidth && LoadedSize.Height == inHeight)
	{
		return;
	}

	// ���� ������ ���
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

	// ���� ������ Ŭ����
	LoadedGridDataContainer.clear();


	// �׸��� �����Ϳ� �޸�
	MMemoryI<1000> readMemory;

	// ���� �׸��� �÷���
	// �̰� �����Ǹ� lefttop�� �׳� ����
	LoadedGridCount = 0;

	// ������ ���鼭 ������ ����
	for (MINT32 x = 0; x < inWidth; ++x)
	{
		for (MINT32 y = 0; y < inHeight; ++y)
		{
			MIntPoint index2(inStartX + x, inStartY + y);
			
			// ��� �׸��� �����͸� ��´�
			MGridData* gridData = nullptr;
			{
				// ��������� �ִ��� üũ�ϰ� �ִٸ� �װ� ���
				auto findIter = backupMap.find(index2);
				if (findIter == backupMap.end())
				{
					// ��� ������ ���°�� ���Ͽ��� �ε�
					MString fileName = GetGridDataFilePath(index2);

					// ��� ������ �ε��غ��� �����Ѵٸ� �װŻ��
					if (MTRUE == MFileUtil::LoadFile(readMemory, fileName))
					{
						// Ǯ���� ������ ������ ���� ī��
						gridData = GridDataPool.Pop();

						MMemoryReadStream readStream(readMemory);
						gridData->Serialize(readStream);
					}
					else
					{
						// ��� ������ ���°��
						if (MTRUE == IsCreateGridDataFile)
						{
							AddNewGridDataFile(fileName, index2);
							continue;
						}
					}
				}
				else
				{
					// ��� ������ �ִ°��
					gridData = findIter->second;
					backupMap.erase(findIter);
				}
			}

			// �Ѿ�� ������ ����(nullptr ����)
			LoadedGridDataContainer.push_back(gridData);
		
			// �׸��� ������ �ִ°�� lefttop�� ����
			if(nullptr != gridData)
			{ 
				if (0 == LoadedGridCount)
				{
					LoaedGridLeftTopPos = gridData->LeftTop;
				}
				else
				{
					// ���� ������ �ƴѰ�� ��
					LoaedGridLeftTopPos.X = MMath::Min(LoaedGridLeftTopPos.X, gridData->LeftTop.X);
					LoaedGridLeftTopPos.Y = MMath::Min(LoaedGridLeftTopPos.Y, gridData->LeftTop.Y);
				}

				++LoadedGridCount;
			}
		}
	}


	// ���� ����
	LoadedStartIndex.Set(inStartX, inStartY);
	LoadedSize.Set(inWidth, inHeight);


	// �̻�� �׸��� ������ Ǯ�� �ݳ�
	for (auto& pair : backupMap) {
		GridDataPool.Push(pair.second);
	}
}


MGridData* MGridDataManager::AddNewGridDataFile(const MString& inFileName, const MIntPoint& inIndex2)
{
	// �ű� ������ ����	
	MGridData* gridData = GridDataPool.Pop();
	gridData->GridIndex2D = inIndex2;

	// ��ġ ����
	MINT32 GridSideSize = GridMetaData.GetGridSideSize();
	gridData->LeftTop.Set(GridSideSize * inIndex2.X, GridSideSize * inIndex2.Y);
	gridData->RightBottom.Set(gridData->LeftTop.X + GridSideSize, gridData->LeftTop.Y + GridSideSize);

	// Ÿ�� ���� �Ҵ�
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

	// ���
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
	// ��Ÿ ���� ����
	if (MFALSE == MFileUtil::SaveToFile(&inMetaData, sizeof(inMetaData), GetMetaFilePath())) {
		return MFALSE;
	}

	// �޸𸮿��� ����
	LoadedGridDataContainer.clear();

	// ��Ÿ ������ �缳��
	GridMetaData = inMetaData;

	return MTRUE;
}


void MGridDataEditManager::UpdateLoadedGridData(std::vector<class MBoxCollider*>& inColliderList)
{
	// �׸��� ������ ����
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
				// �浹�ϴ� �ڽ�
				if (true == MCollision::CheckOBB(tileBoxCollider, *collider))
				{
					tileData->IsObstacle = true;
					break;
				}
			}	
		}

		// ���� ����
		SaveGridData(gridData);
	}
}

void MGridDataEditManager::SaveMetaData()
{
	MString metaFilePath = GetMetaFilePath();


}

void MGridDataEditManager::SaveGridData(MGridData* inGridData)
{
	// �����͸� ����
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