#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE
#include <memory.h>
//////////////////////////////////////////////////////////////////////////
// common define
//����ά��
#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))

#define GAME_PLAYER					3									//��Ϸ����
#define GAME_NAME					TEXT("������")						//��Ϸ����

#define INVALID_WORD				((unsigned short)(0xFFFF))					//��Ч��ֵ
#define INVALID_CHAIR				INVALID_WORD						//��Ч����

//����ģʽ
#define PARAM_MODE_NORMAL           0x00                                //һ��ģʽ
#define PARAM_MODE_LAIZI            0x01                                //���ģʽ
#define PARAM_MODE_QIANG            0x02                                //������

//��Ŀ����
#define MAX_COUNT					20									//�����Ŀ
#define FULL_COUNT					54									//ȫ����Ŀ
#define GOOD_CARD_COUTN				38									//������Ŀ
#define BACK_COUNT					3									//������Ŀ
#define NORMAL_COUNT				17									//������Ŀ

//////////////////////////////////////////////////////////////////////////

//��������
#define ST_ORDER					1									//��С����
#define ST_COUNT					2									//��Ŀ����
#define ST_CUSTOM					3									//�Զ�����

//��ֵ����
#define	MASK_COLOR					0x70								//��ɫ����0xF0
#define	MASK_VALUE					0x0F								//��ֵ����
#define MASK_LAIZI                  0x80                                //�������

//�˿�����
#define CT_ERROR					0									//��������
#define CT_SINGLE					1									//�������� --������ ����� 5
#define CT_DOUBLE					2									//�������� --��ֵ��ͬ�������� ��÷�� 4+ ���� 4
#define CT_THREE					3									//�������� --��ֵ��ͬ�������� ������ J
#define CT_SINGLE_LINE				4									//�������� --���Ż������������ƣ��磺 45678 �� 78910JQK ���������� 2 ���˫��
#define CT_DOUBLE_LINE				5									//�������� --���Ի������������ƣ��磺 334455 �� 7788991010JJ ���������� 2 ���˫��
#define CT_THREE_LINE				6									//�������� --�������������������ƣ��磺 333444 �� 555666777888 ���������� 2 ���˫��
#define CT_THREE_LINE_TAKE_ONE		7									//����һ�� --��ֵ��ͬ�������� + һ�ŵ��ƻ�һ���ơ����磺 333+6 �� 444+99
#define CT_THREE_LINE_TAKE_TWO		8									//����һ��
#define CT_FOUR_LINE_TAKE_ONE		9									//�Ĵ����� --�����ƣ������ơ�ע�⣺�Ĵ�������ը��
#define CT_FOUR_LINE_TAKE_TWO		10									//�Ĵ�����
#define CT_BOMB_CARD				11									//ը������ --����ͬ��ֵ��
#define CT_MISSILE_CARD				12									//������� --˫����������С������������
#define CT_BOMB_SOFT				13									//��ը     --�����Ϊ5�� 333��5�͹�����3333��ը����77��55�͹�����7777��ը��
#define CT_BOMB_PURE				14									//�����ը�� --�����������ɵ�ը��

//////////////////////////////////////////////////////////////////////////

//�����ṹ
struct tagAnalyseResult
{
	unsigned char 							cbBlockCount[4];					//�˿���Ŀ
	unsigned char							cbCardData[4][MAX_COUNT];			//�˿�����
};

//���ƽ��
struct tagOutCardResult
{
	unsigned char							cbCardCount;						//�˿���Ŀ
	unsigned char							cbResultCard[MAX_COUNT];			//����˿�
};

//�ֲ���Ϣ
struct tagDistributing
{
	unsigned char							cbCardCount;						//�˿���Ŀ
	unsigned char							cbDistributing[15][6];				//�ֲ���Ϣ
};

#define MAX_TYPE_COUNT 254
struct tagOutCardTypeResult 
{
	unsigned char							cbCardType;							//�˿�����
	unsigned char							cbCardTypeCount;					//������Ŀ
	unsigned char							cbEachHandCardCount[MAX_TYPE_COUNT];//ÿ�ָ���
	unsigned char							cbCardData[MAX_TYPE_COUNT][MAX_COUNT];//�˿�����
};

struct PokerData{
	PokerData(){
		memset(cardData, 0, sizeof(cardData));
		memset(cbBackCardData, 0, sizeof(cbBackCardData));
		memset(m_cbTurnCardData, 0, sizeof(m_cbTurnCardData));

		m_cbTurnCardCount = 0;
		outPokerUser = 0;
	}

	unsigned char cardData[FULL_COUNT];
	unsigned char cbBackCardData[BACK_COUNT];

	unsigned char m_cbTurnCardCount;				// ��ǰ���Ƹ���
	unsigned char m_cbTurnCardData[MAX_COUNT];		// �����б�
	unsigned char outPokerUser;						// ��ǰ���Ƶ����
};

//��Ϸ�߼���
class CGameLogic
{
	//��������
protected:
	static const unsigned char				m_cbCardData[FULL_COUNT];			//�˿�����
	static const unsigned char				m_cbGoodcardData[GOOD_CARD_COUTN];	//��������

	//��������
public:
	//���캯��
	CGameLogic();
	//��������
	virtual ~CGameLogic();

	//���ͺ���
public:
	//��ȡ����
	unsigned char GetCardType(const unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbLaiziCount = 0);
	//��ȡ��ֵ
	unsigned char GetCardValue(unsigned char cbCardData) { return cbCardData & MASK_VALUE; }
	//��ȡ��ɫ
	unsigned char GetCardColor(unsigned char cbCardData) { return cbCardData & MASK_COLOR; }
	//�Ƿ����
	bool IsLaiziCard(unsigned char cbCardData) { return (cbCardData & MASK_LAIZI) != 0; }

public:
	//�����˿�
	void RandCardList(unsigned char cbCardBuffer[], unsigned char cbBufferCount);
	//�����˿�
	void SortCardList(unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbSortType);
	//ɾ���˿�
	bool RemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, unsigned char cbCardData[], unsigned char cbCardCount);
	//����˿�
	unsigned char GetRandomCard(void);
	//����ɾ���˿�
	bool TryRemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, const unsigned char cbCardData[], unsigned char cbCardCount);

	//�õ�����
	void GetGoodCardData(unsigned char cbGoodCardData[NORMAL_COUNT]) ;
	//ɾ������
	bool RemoveGoodCardData(unsigned char cbGoodcardData[NORMAL_COUNT], unsigned char cbGoodCardCount, unsigned char cbCardData[FULL_COUNT], unsigned char cbCardCount) ;
public:
	//��Ч�ж�
	bool IsValidCard(unsigned char cbCardData);
	//�߼���ֵ
	unsigned char GetCardLogicValue(unsigned char cbCardData);
	//�Ա��˿�
	bool CompareCard(const unsigned char cbFirstCard[], const unsigned char cbNextCard[], unsigned char cbFirstCount, unsigned char cbNextCount);
	//�Ƿ�ը��
	bool IsBombCard(unsigned char cbCardType);
	//�˿�ת��
	unsigned char SwitchToCardIndex(unsigned char cbCardData);

public:
	//�����˿�
	bool AnalysebCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult);
	//�����˿�
	void AnalyseAutoPlayCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult);

//////////////////////////////////////////////////////////////////////////
};


#endif