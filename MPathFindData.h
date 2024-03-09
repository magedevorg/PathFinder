#include "MPrerequisites.h"
#include "MType.h"



// 정보의 구성
// 타일 - 장애물 여부 정보를 가지는 가장 작은단위
// 그리드 - 타일정보의 모음. 여러개의 그리드를 가지고 특정 지역으로 이동시 해당 지역의 그리드만 로드해서 사용.


// 타일 정보
struct MTileData
{
public:
	// 위치 정보
	MIntPoint TileIndex2D;

	// 장애물 여부
	bool IsObstacle = false;
};




// 그리드 정보
class MGridData
{
public:
	//  인덱스 정보
	MIntPoint GridIndex2D;

	// 타일 정보 컨테이너
	std::vector<MTileData> TileDataContainer;
};



struct MPathFindMetaData
{
	// 맵 사이즈(총 사이즈)
	MIntPoint MapSize;

	// 그리드 사이즈
	MINT32 GridSize = 100;

	// 타일 사이즈(cm)
	MINT32 TileSize = 100;
};