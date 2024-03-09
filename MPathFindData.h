#include "MPrerequisites.h"
#include "MType.h"



// ������ ����
// Ÿ�� - ��ֹ� ���� ������ ������ ���� ��������
// �׸��� - Ÿ�������� ����. �������� �׸��带 ������ Ư�� �������� �̵��� �ش� ������ �׸��常 �ε��ؼ� ���.


// Ÿ�� ����
struct MTileData
{
public:
	// ��ġ ����
	MIntPoint TileIndex2D;

	// ��ֹ� ����
	bool IsObstacle = false;
};




// �׸��� ����
class MGridData
{
public:
	//  �ε��� ����
	MIntPoint GridIndex2D;

	// Ÿ�� ���� �����̳�
	std::vector<MTileData> TileDataContainer;
};



struct MPathFindMetaData
{
	// �� ������(�� ������)
	MIntPoint MapSize;

	// �׸��� ������
	MINT32 GridSize = 100;

	// Ÿ�� ������(cm)
	MINT32 TileSize = 100;
};