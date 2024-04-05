#pragma once


#include "MPrerequisites.h"
#include "MVector.h"
#include "MType.h"
#include "MString.h"
#include "MPool.h"



//--------------------------------------------------------------------
// 타일 정보
//--------------------------------------------------------------------
class MTileData
{
public:	
	MIntPoint TileIndex2D;

	bool IsObstacle = false;
};



//--------------------------------------------------------------------
// 그리드 메타 데이터
//--------------------------------------------------------------------
class MGridMetaData
{
public:
	//--------------------------------------------------------
	// 타일 정보
	//--------------------------------------------------------
	MINT32 TileSize;		// 타일 사이즈

	//--------------------------------------------------------
	// 그리드 정보
	//--------------------------------------------------------
	MINT32 GridSideTileCount;	// 그리드 가로 타일 개수

	
	// 그리드 한변 사이즈
	MINT32 GetGridSideSize() const {
		return TileSize * GridSideTileCount;
	}	
};


//--------------------------------------------------------------------
// 하나의 그리드 데이터
//--------------------------------------------------------------------
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




//--------------------------------------------------------------------
// 그리드 관리 클래스
//--------------------------------------------------------------------
class MGridDataManager
{
public:
	MGridDataManager(const MString& inDataPath);
	virtual ~MGridDataManager();

public:
	//-------------------------------------------------------
	// 데이터 로드
	//-------------------------------------------------------
	// 메타 데이터 로드
	MBOOL LoadMetaData();


	virtual MBOOL LoadGridData(const MVector2& inCenterPos, const MINT32 inExtend);

	// 그리드 데이터를 로드
	virtual MBOOL LoadGridData(const MIntPoint& inCenterIndex, const MINT32 inExtend);
	
	
	// 메타 데이터를 얻는다
	MGridMetaData* GetGridMetaData() {
		return &GridMetaData;
	}

	// 데이터 경로
	const MString& GetGridDataPath() const {
		return GridDataPath;
	}

	std::vector<MGridData*>* GetLoadedGridDataContainer() {
		return &LoadedGridDataContainer;
	}

protected:
	// 그리드데이터 로드 로직
	void LoadGridDataLogic(MINT32 inStartX, MINT32 inStartY, MINT32 inWidth, MINT32 inHeight);


	// 메타 파일 패스를 얻는다
	MString GetMetaFilePath() const;

	// 그리드 파일 패스를 얻는다
	MString GetGridDataFilePath(MINT32 inX, MINT32 inY) const;
	MString GetGridDataFilePath(const MIntPoint& inIndex2) const;

protected:
	// 데이터 경로
	MString GridDataPath;

	// 메타 데이터
	MGridMetaData GridMetaData;


	//------------------------------------------------
	// 로드된 정보
	//------------------------------------------------
	MIntPoint LoadedStartIndex;

	// 로드된 사이즈
	MIntSize LoadedSize;

	// 로드된 그리드 데이터
	std::vector<MGridData*> LoadedGridDataContainer;

	// 그리드 데이터 풀
	MPool<MGridData> GridDataPool;


	bool IsCreateGridDataFile;
};




//--------------------------------------------------------------------
// 에디트용 그리드 관리 클래스
//--------------------------------------------------------------------
class MGridDataEditManager : public MGridDataManager
{
public:
	MGridDataEditManager(const MString& inDataPath);

public:
	// 메타 데이터 재설정
	MBOOL ResetMetaData(const MGridMetaData& inMetaData);


	//------------------------------------------------------
	// 저장 처리
	//------------------------------------------------------
	// 메타 데이터 저장
	void SaveMetaData();

	// 그리드 데이터 저장
	void SaveGridData();
};