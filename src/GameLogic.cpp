#include "GameLogic.h"
#include <memory.h>
#include <math.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////
//静态变量

//扑克数据
const unsigned char	CGameLogic::m_cbCardData[FULL_COUNT]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
	0x4E,0x4F,
};

const unsigned char	CGameLogic::m_cbGoodcardData[GOOD_CARD_COUTN]=
{
	0x01,0x02,
	0x11,0x12,
	0x21,0x22,
	0x31,0x32,
	0x4E,0x4F,
	0x07,0x08,0x09,
	0x17,0x18,0x19,
	0x27,0x28,0x29,
	0x37,0x38,0x39,
	0x0A,0x0B,0x0C,0x0D,
	0x1A,0x1B,0x1C,0x1D,
	0x2A,0x2B,0x2C,0x2D,
	0x3A,0x3B,0x3C,0x3D
};

//////////////////////////////////////////////////////////////////////////

//构造函数
CGameLogic::CGameLogic()
{
	//AI变量
}

//析构函数
CGameLogic::~CGameLogic()
{
}

//获取类型
unsigned char CGameLogic::GetCardType(const unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbLaiziCount)
{
	//简单牌型
	switch (cbCardCount)
	{
	case 0:	//空牌
		{
			return CT_ERROR;
		}
	case 1: //单牌
		{
			return CT_SINGLE;
		}
	case 2:	//对牌火箭
		{
			//牌型判断
			if ((cbCardData[0]==0x4F)&&(cbCardData[1]==0x4E)) return CT_MISSILE_CARD;
			if (GetCardLogicValue(cbCardData[0])==GetCardLogicValue(cbCardData[1])) return CT_DOUBLE;

			return CT_ERROR;
		}
	}

	//分析扑克
	tagAnalyseResult AnalyseResult;
	if(!AnalysebCardData(cbCardData,cbCardCount,AnalyseResult)) 
		return CT_ERROR ;

	//四牌判断
	if (AnalyseResult.cbBlockCount[3]>0)
	{
		//牌型判断
		if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==4)) 
		{
			if (cbLaiziCount == 4)
			{
				return CT_BOMB_PURE;
			}
			else if (cbLaiziCount>0)
			{
				return CT_BOMB_SOFT;
			}
			return CT_BOMB_CARD;
		}
		if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==6)) return CT_FOUR_LINE_TAKE_ONE;
		if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==8)&&(AnalyseResult.cbBlockCount[1]==2)) return CT_FOUR_LINE_TAKE_TWO;

		return CT_ERROR;
	}

	//三牌判断
	if (AnalyseResult.cbBlockCount[2]>0)
	{
		//连牌判断
		if (AnalyseResult.cbBlockCount[2]>1)
		{
			//变量定义
			unsigned char cbCardData=AnalyseResult.cbCardData[2][0];
			unsigned char cbFirstLogicValue=GetCardLogicValue(cbCardData);

			//错误过虑
			if (cbFirstLogicValue>=15) return CT_ERROR;

			//连牌判断
			for (unsigned char i=1;i<AnalyseResult.cbBlockCount[2];i++)
			{
				unsigned char cbCardData=AnalyseResult.cbCardData[2][i*3];
				if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
			}
		}
		else if( cbCardCount == 3 ) return CT_THREE;

		//牌形判断
		if (AnalyseResult.cbBlockCount[2]*3==cbCardCount) return CT_THREE_LINE;
		if (AnalyseResult.cbBlockCount[2]*4==cbCardCount) return CT_THREE_LINE_TAKE_ONE;
		if ((AnalyseResult.cbBlockCount[2]*5==cbCardCount)&&(AnalyseResult.cbBlockCount[1]==AnalyseResult.cbBlockCount[2])) return CT_THREE_LINE_TAKE_TWO;

		return CT_ERROR;
	}

	//两张类型
	if (AnalyseResult.cbBlockCount[1]>=3)
	{
		//变量定义
		unsigned char cbCardData=AnalyseResult.cbCardData[1][0];
		unsigned char cbFirstLogicValue=GetCardLogicValue(cbCardData);

		//错误过虑
		if (cbFirstLogicValue>=15) return CT_ERROR;

		//连牌判断
		for (unsigned char i=1;i<AnalyseResult.cbBlockCount[1];i++)
		{
			unsigned char cbCardData=AnalyseResult.cbCardData[1][i*2];
			if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
		}

		//二连判断
		if ((AnalyseResult.cbBlockCount[1]*2)==cbCardCount) return CT_DOUBLE_LINE;

		return CT_ERROR;
	}

	//单张判断
	if ((AnalyseResult.cbBlockCount[0]>=5)&&(AnalyseResult.cbBlockCount[0]==cbCardCount))
	{
		//变量定义
		unsigned char cbCardData=AnalyseResult.cbCardData[0][0];
		unsigned char cbFirstLogicValue=GetCardLogicValue(cbCardData);

		//错误过虑
		if (cbFirstLogicValue>=15) return CT_ERROR;

		//连牌判断
		for (unsigned char i=1;i<AnalyseResult.cbBlockCount[0];i++)
		{
			unsigned char cbCardData=AnalyseResult.cbCardData[0][i];
			if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
		}

		return CT_SINGLE_LINE;
	}

	return CT_ERROR;
}

//排列扑克
void CGameLogic::SortCardList(unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbSortType)
{
	//数目过虑
	if (cbCardCount==0) return;
	if (cbSortType==ST_CUSTOM) return;

	//转换数值
	unsigned char cbSortValue[MAX_COUNT];
	for (unsigned char i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardLogicValue(cbCardData[i]);	

	//排序操作
	bool bSorted=true;
	unsigned char cbSwitchData=0,cbLast=cbCardCount-1;
	do
	{
		bSorted=true;
		for (unsigned char i=0;i<cbLast;i++)
		{
			if ((cbSortValue[i]<cbSortValue[i+1])||
				((cbSortValue[i]==cbSortValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
			{
				//设置标志
				bSorted=false;

				//扑克数据
				cbSwitchData=cbCardData[i];
				cbCardData[i]=cbCardData[i+1];
				cbCardData[i+1]=cbSwitchData;

				//排序权位
				cbSwitchData=cbSortValue[i];
				cbSortValue[i]=cbSortValue[i+1];
				cbSortValue[i+1]=cbSwitchData;
			}	
		}
		cbLast--;
	} while(bSorted==false);

	//数目排序
	if (cbSortType==ST_COUNT)
	{
		//变量定义
		unsigned char cbCardIndex=0;

		//分析扑克
		tagAnalyseResult AnalyseResult;
		AnalysebCardData(&cbCardData[cbCardIndex],cbCardCount-cbCardIndex,AnalyseResult);

		//提取扑克
		for (unsigned char i=0;i<CountArray(AnalyseResult.cbBlockCount);i++)
		{
			//拷贝扑克
			unsigned char cbIndex=CountArray(AnalyseResult.cbBlockCount)-i-1;
			memcpy(&cbCardData[cbCardIndex],AnalyseResult.cbCardData[cbIndex],AnalyseResult.cbBlockCount[cbIndex]*(cbIndex+1)*sizeof(unsigned char));

			//设置索引
			cbCardIndex+=AnalyseResult.cbBlockCount[cbIndex]*(cbIndex+1)*sizeof(unsigned char);
		}
	}

	return;
}

//得到好牌
void CGameLogic::GetGoodCardData(unsigned char cbGoodCardData[NORMAL_COUNT])
{
	//混乱准备
	unsigned char cbCardData[CountArray(m_cbGoodcardData)];
	unsigned char cbCardBuffer[CountArray(m_cbGoodcardData)];
	memcpy(cbCardData, m_cbGoodcardData, sizeof(m_cbGoodcardData));

	//混乱扑克
	unsigned char cbRandCount=0,cbPosition=0;
	unsigned char cbBufferCount=CountArray(m_cbGoodcardData);
	do
	{
		cbPosition=rand()%(cbBufferCount-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[cbBufferCount-cbRandCount];
	} while (cbRandCount<cbBufferCount);

	//复制好牌
	memcpy(cbGoodCardData, cbCardBuffer, NORMAL_COUNT);
}

//删除好牌
bool CGameLogic::RemoveGoodCardData(unsigned char cbGoodcardData[NORMAL_COUNT], unsigned char cbGoodCardCount, unsigned char cbCardData[FULL_COUNT], unsigned char cbCardCount) 
{
	if(cbGoodCardCount>cbCardCount)
		return false ;

	//定义变量
	unsigned char cbDeleteCount=0,cbTempCardData[FULL_COUNT];
	if (cbCardCount>CountArray(cbTempCardData)) return false;
	memcpy(cbTempCardData, cbCardData, cbCardCount*sizeof(cbCardData[0]));

	//置零扑克
	for (unsigned char i=0;i<cbGoodCardCount;i++)
	{
		for (unsigned char j=0;j<cbCardCount;j++)
		{
			if (cbGoodcardData[i]==cbTempCardData[j])
			{
				cbDeleteCount++;
				cbTempCardData[j]=0;
				break;
			}
		}
	}
	if (cbDeleteCount!=cbGoodCardCount) return false;

	//清理扑克
	unsigned char cbCardPos=0;
	for (unsigned char i=0;i<cbCardCount;i++)
	{
		if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
	}

	return true;
}

//扑克转换
unsigned char CGameLogic::SwitchToCardIndex(unsigned char cbCardData)
{
	if (cbCardData == 0x4E) return 52;
	if (cbCardData == 0x4F) return 53;
	return ((cbCardData&MASK_COLOR)>>4) * 13 + (cbCardData&MASK_VALUE)-1;
}

//混乱扑克
void CGameLogic::RandCardList(unsigned char cbCardBuffer[], unsigned char cbBufferCount)
{
	//混乱准备
	unsigned char cbCardData[CountArray(m_cbCardData)];
	memcpy(cbCardData,m_cbCardData,sizeof(m_cbCardData));

	//混乱扑克
	unsigned char cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=rand()%(cbBufferCount-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[cbBufferCount-cbRandCount];
	} while (cbRandCount<cbBufferCount);

	return;
}

//尝试删除扑克
bool CGameLogic::TryRemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, const unsigned char cbCardData[], unsigned char cbCardCount)
{
	//检验数据
	if(cbRemoveCount>cbCardCount)
		return false ;
	if (cbRemoveCount == 0) return true;

	//定义变量
	unsigned char cbDeleteCount=0,cbTempCardData[MAX_COUNT];
	if (cbCardCount>CountArray(cbTempCardData)) return false;
	memcpy(cbTempCardData, cbCardData, cbCardCount*sizeof(cbCardData[0]));

	//置零扑克
	for (unsigned char i=0;i<cbRemoveCount;i++)
	{
		for (unsigned char j=0;j<cbCardCount;j++)
		{
			if (cbRemoveCard[i]==cbTempCardData[j])
			{
				cbDeleteCount++;
				cbTempCardData[j]=0;
				break;
			}
		}
	}
	if (cbDeleteCount != cbRemoveCount) 
	{
		return false;
	}

	//清理扑克
	//unsigned char cbCardPos=0;
	//for (unsigned char i=0;i<cbCardCount;i++)
	//{
	//	if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
	//}

	return true;
}

//删除扑克
bool CGameLogic::RemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, unsigned char cbCardData[], unsigned char cbCardCount)
{
	//检验数据
	if(cbRemoveCount>cbCardCount)
		return false ;
	if (cbRemoveCount == 0) return true;

	//定义变量
	unsigned char cbDeleteCount=0,cbTempCardData[MAX_COUNT];
	if (cbCardCount>CountArray(cbTempCardData)) return false;
	memcpy(cbTempCardData, cbCardData, cbCardCount*sizeof(cbCardData[0]));

	//置零扑克
	for (unsigned char i=0;i<cbRemoveCount;i++)
	{
		for (unsigned char j=0;j<cbCardCount;j++)
		{
			if (cbRemoveCard[i]==cbTempCardData[j])
			{
				cbDeleteCount++;
				cbTempCardData[j]=0;
				break;
			}
		}
	}
	if (cbDeleteCount != cbRemoveCount) 
	{
		return false;
	}

	//清理扑克
	unsigned char cbCardPos=0;
	for (unsigned char i=0;i<cbCardCount;i++)
	{
		if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
	}

	return true;
}

//有效判断
bool CGameLogic::IsValidCard(unsigned char cbCardData)
{
	//获取属性
	unsigned char cbCardColor=GetCardColor(cbCardData);
	unsigned char cbCardValue=GetCardValue(cbCardData);

	//有效判断
	if ((cbCardData==0x4E)||(cbCardData==0x4F)) return true;
	if ((cbCardColor<=0x30)&&(cbCardValue>=0x01)&&(cbCardValue<=0x0D)) return true;

	return false;
}

//是否炸弹
bool CGameLogic::IsBombCard(unsigned char cbCardType)
{
	switch (cbCardType)
	{
	case CT_BOMB_SOFT:
	case CT_BOMB_CARD:
	case CT_BOMB_PURE:
	case CT_MISSILE_CARD:
		return true;
	}
	return false;
}

//逻辑数值
unsigned char CGameLogic::GetCardLogicValue(unsigned char cbCardData)
{
	//扑克属性
	unsigned char cbCardColor=GetCardColor(cbCardData);
	unsigned char cbCardValue=GetCardValue(cbCardData);

	//转换数值
	if (cbCardColor == 0x40) return cbCardValue + 2;
	return (cbCardValue<=2)?(cbCardValue+13):cbCardValue;
}

//Next>First true
//else false
//对比扑克
bool CGameLogic::CompareCard(const unsigned char cbFirstCard[], const unsigned char cbNextCard[], unsigned char cbFirstCount, unsigned char cbNextCount)
{
	//获取类型
	unsigned char cbNextType=GetCardType(cbNextCard,cbNextCount);
	unsigned char cbFirstType=GetCardType(cbFirstCard,cbFirstCount);

	//类型判断
	//火箭判断
	if (cbNextType==CT_MISSILE_CARD) return true;
	if (cbFirstType==CT_MISSILE_CARD) return false ;

	//纯癞子炸弹最大
	if (cbNextType==CT_BOMB_PURE) return true;
	if (cbFirstType==CT_BOMB_PURE) return false;

	//软炸弹最小
	if (cbFirstType == CT_BOMB_SOFT && cbNextType == CT_BOMB_CARD) return true;
	if (cbFirstType == CT_BOMB_CARD && cbNextType == CT_BOMB_SOFT) return false;

	//炸弹判断
	if ((cbFirstType!=CT_BOMB_CARD)&&(cbNextType==CT_BOMB_CARD)) return true;
	if ((cbFirstType==CT_BOMB_CARD)&&(cbNextType!=CT_BOMB_CARD)) return false;

	if (IsBombCard(cbFirstType) && !IsBombCard(cbNextType)) return false;
	if (!IsBombCard(cbFirstType) && IsBombCard(cbNextType)) return true;

	//规则判断
	if ((cbFirstType!=cbNextType)||(cbFirstCount!=cbNextCount)) return false;

	//开始对比
	switch (cbNextType)
	{
	case CT_SINGLE:
	case CT_DOUBLE:
	case CT_THREE:
	case CT_SINGLE_LINE:
	case CT_DOUBLE_LINE:
	case CT_THREE_LINE:
	case CT_BOMB_CARD:
		{
			//获取数值
			unsigned char cbNextLogicValue=GetCardLogicValue(cbNextCard[0]);
			unsigned char cbFirstLogicValue=GetCardLogicValue(cbFirstCard[0]);

			//对比扑克
			return cbNextLogicValue>cbFirstLogicValue;
		}
	case CT_THREE_LINE_TAKE_ONE:
	case CT_THREE_LINE_TAKE_TWO:
		{
			//分析扑克
			tagAnalyseResult NextResult;
			tagAnalyseResult FirstResult;
			AnalysebCardData(cbNextCard,cbNextCount,NextResult);
			AnalysebCardData(cbFirstCard,cbFirstCount,FirstResult);

			//获取数值
			unsigned char cbNextLogicValue=GetCardLogicValue(NextResult.cbCardData[2][0]);
			unsigned char cbFirstLogicValue=GetCardLogicValue(FirstResult.cbCardData[2][0]);

			//对比扑克
			return cbNextLogicValue>cbFirstLogicValue;
		}
	case CT_FOUR_LINE_TAKE_ONE:
	case CT_FOUR_LINE_TAKE_TWO:
		{
			//分析扑克
			tagAnalyseResult NextResult;
			tagAnalyseResult FirstResult;
			AnalysebCardData(cbNextCard,cbNextCount,NextResult);
			AnalysebCardData(cbFirstCard,cbFirstCount,FirstResult);

			//获取数值
			unsigned char cbNextLogicValue=GetCardLogicValue(NextResult.cbCardData[3][0]);
			unsigned char cbFirstLogicValue=GetCardLogicValue(FirstResult.cbCardData[3][0]);

			//对比扑克
			return cbNextLogicValue>cbFirstLogicValue;
		}
	}

	return false;
}

//分析扑克
bool CGameLogic::AnalysebCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult)
{
	//设置结果
	memset(&AnalyseResult,0,sizeof(AnalyseResult));

	//扑克分析
	for (unsigned char i=0;i<cbCardCount;i++)
	{
		//变量定义
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbCardData[i]);
		if(cbLogicValue<=0) 
			return false;

		//搜索同牌
		for (unsigned char j=i+1;j<cbCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbCardData[j])!=cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}

		//设置结果
		unsigned char cbIndex=AnalyseResult.cbBlockCount[cbSameCount-1]++;
		for (unsigned char j=0;j<cbSameCount;j++) AnalyseResult.cbCardData[cbSameCount-1][cbIndex*cbSameCount+j]=cbCardData[i+j];

		//设置索引
		i+=cbSameCount-1;
	}

	return true;
}
//随机扑克
unsigned char CGameLogic::GetRandomCard(void)
{
	size_t cbIndex = rand()%(sizeof(m_cbCardData)) ;
	return m_cbCardData[cbIndex] ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////