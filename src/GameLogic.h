#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE
#include <memory.h>
//////////////////////////////////////////////////////////////////////////
// common define
//数组维数
#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))

#define GAME_PLAYER					3									//游戏人数
#define GAME_NAME					TEXT("斗地主")						//游戏名字

#define INVALID_WORD				((unsigned short)(0xFFFF))					//无效数值
#define INVALID_CHAIR				INVALID_WORD						//无效椅子

//配置模式
#define PARAM_MODE_NORMAL           0x00                                //一般模式
#define PARAM_MODE_LAIZI            0x01                                //癞子模式
#define PARAM_MODE_QIANG            0x02                                //抢地主

//数目定义
#define MAX_COUNT					20									//最大数目
#define FULL_COUNT					54									//全牌数目
#define GOOD_CARD_COUTN				38									//好牌数目
#define BACK_COUNT					3									//底牌数目
#define NORMAL_COUNT				17									//常规数目

//////////////////////////////////////////////////////////////////////////

//排序类型
#define ST_ORDER					1									//大小排序
#define ST_COUNT					2									//数目排序
#define ST_CUSTOM					3									//自定排序

//数值掩码
#define	MASK_COLOR					0x70								//花色掩码0xF0
#define	MASK_VALUE					0x0F								//数值掩码
#define MASK_LAIZI                  0x80                                //癞子掩码

//扑克类型
#define CT_ERROR					0									//错误类型
#define CT_SINGLE					1									//单牌类型 --单个牌 如红桃 5
#define CT_DOUBLE					2									//对牌类型 --数值相同的两张牌 如梅花 4+ 方块 4
#define CT_THREE					3									//三条类型 --数值相同的三张牌 如三个 J
#define CT_SINGLE_LINE				4									//单连类型 --五张或更多的连续单牌（如： 45678 或 78910JQK ）。不包括 2 点和双王
#define CT_DOUBLE_LINE				5									//对连类型 --三对或更多的连续对牌（如： 334455 、 7788991010JJ ）。不包括 2 点和双王
#define CT_THREE_LINE				6									//三连类型 --二个或更多的连续三张牌（如： 333444 、 555666777888 ）。不包括 2 点和双王
#define CT_THREE_LINE_TAKE_ONE		7									//三带一单 --数值相同的三张牌 + 一张单牌或一对牌。例如： 333+6 或 444+99
#define CT_THREE_LINE_TAKE_TWO		8									//三带一对
#define CT_FOUR_LINE_TAKE_ONE		9									//四带两单 --四张牌＋两手牌。注意：四带二不是炸弹
#define CT_FOUR_LINE_TAKE_TWO		10									//四带两对
#define CT_BOMB_CARD				11									//炸弹类型 --四张同数值牌
#define CT_MISSILE_CARD				12									//火箭类型 --双王（大王和小王），最大的牌
#define CT_BOMB_SOFT				13									//软炸     --如癞子为5， 333＋5就构成了3333软炸弹。77＋55就构成了7777软炸弹
#define CT_BOMB_PURE				14									//纯癞子炸弹 --四张癞子牌组成的炸弹

//////////////////////////////////////////////////////////////////////////

//分析结构
struct tagAnalyseResult
{
	unsigned char 							cbBlockCount[4];					//扑克数目
	unsigned char							cbCardData[4][MAX_COUNT];			//扑克数据
};

//出牌结果
struct tagOutCardResult
{
	unsigned char							cbCardCount;						//扑克数目
	unsigned char							cbResultCard[MAX_COUNT];			//结果扑克
};

//分布信息
struct tagDistributing
{
	unsigned char							cbCardCount;						//扑克数目
	unsigned char							cbDistributing[15][6];				//分布信息
};

#define MAX_TYPE_COUNT 254
struct tagOutCardTypeResult 
{
	unsigned char							cbCardType;							//扑克类型
	unsigned char							cbCardTypeCount;					//牌型数目
	unsigned char							cbEachHandCardCount[MAX_TYPE_COUNT];//每手个数
	unsigned char							cbCardData[MAX_TYPE_COUNT][MAX_COUNT];//扑克数据
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

	unsigned char m_cbTurnCardCount;				// 当前出牌个数
	unsigned char m_cbTurnCardData[MAX_COUNT];		// 出牌列表
	unsigned char outPokerUser;						// 当前出牌的玩家
};

//游戏逻辑类
class CGameLogic
{
	//变量定义
protected:
	static const unsigned char				m_cbCardData[FULL_COUNT];			//扑克数据
	static const unsigned char				m_cbGoodcardData[GOOD_CARD_COUTN];	//好牌数据

	//函数定义
public:
	//构造函数
	CGameLogic();
	//析构函数
	virtual ~CGameLogic();

	//类型函数
public:
	//获取类型
	unsigned char GetCardType(const unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbLaiziCount = 0);
	//获取数值
	unsigned char GetCardValue(unsigned char cbCardData) { return cbCardData & MASK_VALUE; }
	//获取花色
	unsigned char GetCardColor(unsigned char cbCardData) { return cbCardData & MASK_COLOR; }
	//是否癞子
	bool IsLaiziCard(unsigned char cbCardData) { return (cbCardData & MASK_LAIZI) != 0; }

public:
	//混乱扑克
	void RandCardList(unsigned char cbCardBuffer[], unsigned char cbBufferCount);
	//排列扑克
	void SortCardList(unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbSortType);
	//删除扑克
	bool RemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, unsigned char cbCardData[], unsigned char cbCardCount);
	//随机扑克
	unsigned char GetRandomCard(void);
	//尝试删除扑克
	bool TryRemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, const unsigned char cbCardData[], unsigned char cbCardCount);

	//得到好牌
	void GetGoodCardData(unsigned char cbGoodCardData[NORMAL_COUNT]) ;
	//删除好牌
	bool RemoveGoodCardData(unsigned char cbGoodcardData[NORMAL_COUNT], unsigned char cbGoodCardCount, unsigned char cbCardData[FULL_COUNT], unsigned char cbCardCount) ;
public:
	//有效判断
	bool IsValidCard(unsigned char cbCardData);
	//逻辑数值
	unsigned char GetCardLogicValue(unsigned char cbCardData);
	//对比扑克
	bool CompareCard(const unsigned char cbFirstCard[], const unsigned char cbNextCard[], unsigned char cbFirstCount, unsigned char cbNextCount);
	//是否炸弹
	bool IsBombCard(unsigned char cbCardType);
	//扑克转换
	unsigned char SwitchToCardIndex(unsigned char cbCardData);

public:
	//分析扑克
	bool AnalysebCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult);
	//分析扑克
	void AnalyseAutoPlayCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult);

//////////////////////////////////////////////////////////////////////////
};


#endif