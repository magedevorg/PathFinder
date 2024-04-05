#include "MGridDataManager.h"
#include "MFileUtil.h"


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
	// ��� Ǯ�� �ݳ�
	const MINT32 count = LoadedGridDataContainer.size();
	for (MINT32 i = 0; i < count; ++i) {
		GridDataPool.Push(LoadedGridDataContainer[i]);
	}
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


MBOOL MGridDataManager::LoadGridData(const MVector2& inCenterPos, const MINT32 inExtend)
{
	MINT32 gridSideSize = GridMetaData.GetGridSideSize();

	// ��ġ -> �ε��� ���� ����
	auto ConvertIndexFunc = [gridSideSize](MFLOAT inValue)->MINT32
		{
			int32 index = inValue / gridSideSize;
			if (inValue < 0) {
				index -= 1;
			}

			return index;
		};


	// ������ �ε����� ��´�
	const int32 indexX = ConvertIndexFunc(inCenterPos.X);
	const int32 indexY = ConvertIndexFunc(inCenterPos.Y);
	
	return LoadGridData(MIntPoint(indexX, indexY), inExtend);
}


MBOOL MGridDataManager::LoadGridData(const MIntPoint& inCenterIndex, const MINT32 inExtend)
{
	MINT32 size = (inExtend * 2) + 1;
	LoadGridDataLogic(inCenterIndex.X - inExtend, inCenterIndex.Y - inExtend, size, size);
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
	MMemoryI<sizeof(MGridData)> readMemory;

	// ������ ���鼭 ������ ����
	for (MINT32 x = 0; x < inWidth; ++x)
	{
		for (MINT32 y = 0; y < inHeight; ++y)
		{
			MIntPoint index2(inStartX + x, inStartY + y);
			
			
			// ��������� �ִ��� üũ�ϰ� �ִٸ� �װ� ���
			auto findIter = backupMap.find(index2);
			if (findIter != backupMap.end())
			{
				LoadedGridDataContainer.push_back(findIter->second);
				backupMap.erase(findIter);
				continue;
			}


			// ����� ���°�� ���Ͽ��� �ҷ��´�
			MString fileName = GetGridDataFilePath(index2);

			// ��� ������ �ε��غ��� �����Ѵٸ� �װŻ��
			if ( MTRUE == MFileUtil::LoadFile(readMemory, fileName) )
			{
				// Ǯ���� ������ ������ ���� ī��
				MGridData* gridData = GridDataPool.Pop();
				readMemory.Read(gridData, 0, sizeof(MGridData));

				LoadedGridDataContainer.push_back(gridData);
				continue;
			}


			if (MTRUE == IsCreateGridDataFile)
			{
				// �ű� ������ ����	
				MGridData* gridData = GridDataPool.Pop();
				gridData->GridIndex2D = index2;

				// ��ġ ����
				MINT32 GridSideSize = GridMetaData.GetGridSideSize();
				gridData->LeftTop.X = GridSideSize * index2.X;
				gridData->LeftTop.Y = GridSideSize * index2.Y;

				LoadedGridDataContainer.push_back(gridData);

				MFileUtil::SaveToFile(gridData, sizeof(MGridData), fileName);
				continue;
			}
			
			// �̰͵� ���͵� �ƴϸ� nullptr �߰�
			LoadedGridDataContainer.push_back(nullptr);
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

void MGridDataEditManager::SaveMetaData()
{
	MString metaFilePath = GetMetaFilePath();


}

void MGridDataEditManager::SaveGridData()
{

}

