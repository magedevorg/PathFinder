﻿#pragma once


#include "MPrerequisites.h"
#include "MVector.h"
#include "MType.h"
#include "MString.h"
#include "MPool.h"
#include "MStream.h"




//--------------------------------------------------------------------
// 그리드 메타 데이터
//--------------------------------------------------------------------
class MGridMetaData
{
public:
	


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
// 타일 정보
//--------------------------------------------------------------------
class MTileData
{
public:
	MIntPoint TileIndex2D;

	bool IsObstacle = false;
};



//--------------------------------------------------------------------
// 하나의 그리드 데이터
//--------------------------------------------------------------------
class MGridData : public MSerializable
{
public:
	// 직렬화/역직렬화
	virtual void Serialize(class MStream& inStream) override;

	// 타일 데이터를 얻는다
	MTileData* GetTileData(MINT32 inIndex);
	


public:
	// 인덱스
	MIntPoint GridIndex2D;

	// 위치 정보
	MVector2 LeftTop;
	MVector2 RightBottom;

	// 타일 데이터
	MMemory TileDataContainer;
};




//--------------------------------------------------------------------
// 그리드 관리 클래스
//--------------------------------------------------------------------
class MGridDataManager
{
public:
	MGridDataManager();
	virtual ~MGridDataManager();

public:
	// 그리드 데이터 매니저 초기화
	virtual MBOOL InitGridDataManager(const MString& inDataPath);

	//-------------------------------------------------------
	// 데이터 로드
	//-------------------------------------------------------
	// 메타 데이터 로드
	MBOOL LoadMetaData();


	virtual MBOOL LoadGridDataByPosition(const MVector2& inCenterPos, const MINT32 inExtend);

	// 그리드 데이터를 로드
	virtual MBOOL LoadGridDataByIndex(const MIntPoint& inCenterIndex, const MINT32 inExtend);

	// 위치로 인덱스를 얻는다
	MBOOL GetIndex2DByPosition(MIntPoint& inIndex2D, const MVector2& inPos);
	
	// 타일 데이터를 얻는다
	MTileData* GetTileDataByIndex2D(const MIntPoint& inIndex2D);

	// 
	MBOOL GetTileLeftTopPositionByIndex(MVector2& inPos, const MIntPoint& inIndex2D);
	
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


	const MVector2& GetLoaedGridLeftTopPos() const {
		return LoaedGridLeftTopPos;
	}


protected:
	// 그리드데이터 로드 로직
	void LoadGridDataLogic(MINT32 inStartX, MINT32 inStartY, MINT32 inWidth, MINT32 inHeight);

	// 신규 그리드 데이터를 추가
	MGridData* AddNewGridDataFile(const MString& inFileName, const MIntPoint& inIndex2);


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

	// 로드된 그리드의 왼쪽위 위치
	MVector2 LoaedGridLeftTopPos;

	// 로드된 그리드 개수
	MINT32 LoadedGridCount = 0;

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
	MGridDataEditManager();

public:

	// 메타 데이터 재설정
	MBOOL ResetMetaData(const MGridMetaData& inMetaData);

	// 현재 로드된 그리드데이터를 갱신
	void UpdateLoadedGridData(std::vector<class MBoxCollider*>& inColliderList);



	//------------------------------------------------------
	// 저장 처리
	//------------------------------------------------------
	// 메타 데이터 저장
	void SaveMetaData();

	// 그리드 데이터 저장
	void SaveGridData(MGridData* inGridData);


protected:
	// 타일의 중심 위치를 얻는다
	MVector3 GetTileCenterPosition(MGridData* inGridData, MTileData* inTileData);
};