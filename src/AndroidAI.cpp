#include "AndroidAI.h"
#define ASSERT(n)

static void my_memset(void* ptr,size_t cnt){
	memset(ptr, 0, cnt);
}

CAndroidAI::CAndroidAI()
{
	my_memset(m_cbAllCardData,sizeof(m_cbAllCardData));
#if 0
	my_memset(m_cbLandScoreCardData,sizeof(m_cbLandScoreCardData));
#endif
	my_memset(m_cbUserCardCount, sizeof(m_cbUserCardCount));
	m_wBankerUser = INVALID_CHAIR;
	m_lBankerOutCardCount = 0;
}

CAndroidAI::~CAndroidAI()
{
}

//获取类型
unsigned char CAndroidAI::GetCardType(const unsigned char cbCardData[], unsigned char cbCardCount)
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
			if ((cbCardData[1] == 0x4F) && (cbCardData[0] == 0x4E)) return CT_MISSILE_CARD;

			if (GetCardLogicValue(cbCardData[0])==GetCardLogicValue(cbCardData[1])) return CT_DOUBLE;

			return CT_ERROR;
		}
	}

	//分析扑克
	tagAnalyseResult AnalyseResult;
	AnalysebCardData(cbCardData,cbCardCount,AnalyseResult);

	//四牌判断
	if (AnalyseResult.cbBlockCount[3]>0)
	{
		//牌型判断
		if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==4)) return CT_BOMB_CARD;
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

//逻辑数值
unsigned char CAndroidAI::GetCardLogicValue(unsigned char cbCardData)
{
	//扑克属性
	unsigned char cbCardColor=GetCardColor(cbCardData);
	unsigned char cbCardValue=GetCardValue(cbCardData);


	if(cbCardValue<=0 || cbCardValue>(MASK_VALUE&0x4f))
		return 0 ;


	//转换数值
	if (cbCardColor==0x40) return cbCardValue+2;
	return (cbCardValue<=2)?(cbCardValue+13):cbCardValue;
}

//有效判断
bool CAndroidAI::IsValidCard(unsigned char cbCardData)
{
	//获取属性
	unsigned char cbCardColor=GetCardColor(cbCardData);
	unsigned char cbCardValue=GetCardValue(cbCardData);

	//有效判断
	if ((cbCardData==0x4E)||(cbCardData==0x4F)) return true;
	if ((cbCardColor<=0x30)&&(cbCardValue>=0x01)&&(cbCardValue<=0x0D)) return true;

	return false;
}

//对比扑克
bool CAndroidAI::CompareCard(const unsigned char cbFirstCard[], const unsigned char cbNextCard[], unsigned char cbFirstCount, unsigned char cbNextCount)
{
	//获取类型
	unsigned char cbNextType=GetCardType(cbNextCard,cbNextCount);
	unsigned char cbFirstType=GetCardType(cbFirstCard,cbFirstCount);

	//类型判断
	if (cbNextType==CT_ERROR) return false;
	if (cbNextType==CT_MISSILE_CARD) return true;

	//炸弹判断
	if ((cbFirstType!=CT_BOMB_CARD)&&(cbNextType==CT_BOMB_CARD)) return true;
	if ((cbFirstType==CT_BOMB_CARD)&&(cbNextType!=CT_BOMB_CARD)) return false;

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

//清空数据
void CAndroidAI::ClearAllCard()
{
	my_memset(m_cbAllCardData,sizeof(m_cbAllCardData));
	my_memset(m_cbUserCardCount, sizeof(m_cbUserCardCount));
	m_wBankerUser = INVALID_CHAIR;
	m_lBankerOutCardCount = 0;
}


//叫分判断
unsigned char CAndroidAI::AnalyseLandScore(int chairIdx, unsigned char cbBackCardData[])
{
	unsigned char cbLandScoreCardData[MAX_COUNT];	//叫牌扑克
	memcpy(cbLandScoreCardData, m_cbAllCardData[chairIdx], NORMAL_COUNT);
	memcpy(&cbLandScoreCardData[NORMAL_COUNT], cbBackCardData, BACK_COUNT);

	//大牌数目
	unsigned char cbLargeCardCount = 0 ;
	unsigned char Index=0 ;
	while (GetCardLogicValue(cbLandScoreCardData[Index++]) >= 15)
		++cbLargeCardCount ;

	//单牌个数
	unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbLandScoreCardData, sizeof(cbLandScoreCardData), NULL, 0);

	//叫两分
	if(cbLargeCardCount >= 4 && cbSingleCardCount <= 4) 
		return 3 ;

	//放弃叫分
	if(cbLargeCardCount <= 2) 
		return 255 ;

	//其他单牌
	unsigned char cbMinSingleCardCount = MAX_COUNT ;
	for(int wChairID=0 , i=0; wChairID < GAME_PLAYER; ++wChairID){
		if (wChairID != chairIdx){
			unsigned char cbTmpSingleCardCount = AnalyseSinleCardCount(m_cbAllCardData[wChairID], NORMAL_COUNT, NULL, 0);
			if (cbTmpSingleCardCount < cbMinSingleCardCount){
				cbTmpSingleCardCount = cbMinSingleCardCount;
			}
		}	
	}

	//叫一分
	if(cbLargeCardCount >= 3 && cbSingleCardCount < cbMinSingleCardCount - 3)
		return 2 ;

	//放弃叫分
	return 255 ;
}

//排列扑克
void CAndroidAI::SortCardList(unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbSortType)
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

//删除扑克
bool CAndroidAI::RemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, unsigned char cbCardData[], unsigned char cbCardCount)
{
	//检验数据
	if(cbRemoveCount>cbCardCount)
		return false ;

	//定义变量
	unsigned char cbDeleteCount=0,cbTempCardData[MAX_COUNT];
	if (cbCardCount>CountArray(cbTempCardData)) return false;
	memcpy(cbTempCardData,cbCardData,cbCardCount*sizeof(cbCardData[0]));

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
	if (cbDeleteCount!=cbRemoveCount) return false;

	//清理扑克
	unsigned char cbCardPos=0;
	for (unsigned char i=0;i<cbCardCount;i++)
	{
		if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
	}

	return true;
}

//分析扑克
void CAndroidAI::AnalysebCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult)
{
	//设置结果
	my_memset(&AnalyseResult,sizeof(AnalyseResult));

	//扑克分析
	for (unsigned char i=0;i<cbCardCount;i++)
	{
		//变量定义
		unsigned char cbSameCount=1,cbCardValueTemp=0;
		unsigned char cbLogicValue=GetCardLogicValue(cbCardData[i]);

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

	return;
}

//设置扑克
void CAndroidAI::SetUserCard(int wChairID, unsigned char cbCardData[], unsigned char cbCardCount)
{
	memcpy(m_cbAllCardData[wChairID], cbCardData, cbCardCount*sizeof(unsigned char)) ;
	m_cbUserCardCount[wChairID] = cbCardCount ;

	//排列扑克
	SortCardList(m_cbAllCardData[wChairID], cbCardCount, ST_ORDER) ;
}

//设置底牌
void CAndroidAI::SetBackCard(int wChairID, unsigned char cbBackCardData[], unsigned char cbCardCount)
{
	unsigned char cbTmpCount = m_cbUserCardCount[wChairID] ;
	memcpy(m_cbAllCardData[wChairID]+cbTmpCount, cbBackCardData, cbCardCount*sizeof(unsigned char)) ;
	m_cbUserCardCount[wChairID] += cbCardCount ;

	//排列扑克
	SortCardList(m_cbAllCardData[wChairID], m_cbUserCardCount[wChairID], ST_ORDER) ;
}

//设置庄家
void CAndroidAI::SetBanker(int wBanker) 
{
	m_wBankerUser = wBanker ;
}
//叫牌扑克
#if 0
void CAndroidAI::SetLandScoreCardData(unsigned char cbCardData[], unsigned char cbCardCount) 
{
	if(cbCardCount!=MAX_COUNT) return ;

	memcpy(m_cbLandScoreCardData, cbCardData, cbCardCount*sizeof(unsigned char)) ;
	//排列扑克
	SortCardList(m_cbLandScoreCardData, cbCardCount, ST_ORDER) ;
}
#endif

//删除扑克
void CAndroidAI::RemoveUserCardData(int wChairID, unsigned char cbRemoveCardData[], unsigned char cbRemoveCardCount) 
{
	bool bSuccess = RemoveCard(cbRemoveCardData, cbRemoveCardCount, m_cbAllCardData[wChairID], m_cbUserCardCount[wChairID]) ;
	m_cbUserCardCount[wChairID] -= cbRemoveCardCount;
}

////以下为AI函数

//出牌搜索
bool CAndroidAI::SearchOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, int wOutCardUser, int wMeChairID, tagOutCardResult & OutCardResult)
{
	//玩家判断
	int wUndersideOfBanker = (m_wBankerUser+1)%GAME_PLAYER ;	//地主下家
	int wUpsideOfBanker = (wUndersideOfBanker+1)%GAME_PLAYER ;	//地主上家

	//初始变量
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//先出牌
	if(cbTurnCardCount==0)
	{
		//地主出牌
		if(wMeChairID==m_wBankerUser) BankerOutCard(cbHandCardData, cbHandCardCount, OutCardResult) ;
		//地主下家
		else if(wMeChairID==wUndersideOfBanker) UndersideOfBankerOutCard(cbHandCardData, cbHandCardCount,wMeChairID,  OutCardResult) ;
		//地主上家
		else if(wMeChairID==wUpsideOfBanker) UpsideOfBankerOutCard(cbHandCardData, cbHandCardCount, wMeChairID, OutCardResult) ;
	}
	//压牌
	else
	{	
		//地主出牌
		if(wMeChairID==m_wBankerUser) BankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount, OutCardResult) ;
		//地主下家
		else if(wMeChairID==wUndersideOfBanker) UndersideOfBankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount, OutCardResult) ;
		//地主上家
		else if(wMeChairID==wUpsideOfBanker) UpsideOfBankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount, OutCardResult) ;
	}
	return true ;
}

//分析炸弹
void CAndroidAI::GetAllBomCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbBomCardData[], unsigned char &cbBomCardCount)
{
	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//大小排序
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbBomCardCount = 0 ;

	if(cbHandCardCount<2) return ;

	//双王炸弹
	if(0x4F==cbTmpCardData[0] && 0x4E==cbTmpCardData[1])
	{
		cbBomCardData[cbBomCardCount++] = cbTmpCardData[0] ;
		cbBomCardData[cbBomCardCount++] = cbTmpCardData[1] ;
	}

	//扑克分析
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//变量定义
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//搜索同牌
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}

		if(4==cbSameCount)
		{
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i] ;
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i+1] ;
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i+2] ;
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i+3] ;
		}

		//设置索引
		i+=cbSameCount-1;
	}
}

//分析顺子
void CAndroidAI::GetAllLineCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbLineCardData[], unsigned char &cbLineCardCount)
{
	unsigned char cbTmpCard[MAX_COUNT] ;
	memcpy(cbTmpCard, cbHandCardData, cbHandCardCount) ;
	//大小排序
	SortCardList(cbTmpCard, cbHandCardCount, ST_ORDER) ;

	cbLineCardCount = 0 ;

	//数据校验
	if(cbHandCardCount<5) return ;

	unsigned char cbFirstCard = 0 ;
	//去除2和王
	for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

	unsigned char cbSingleLineCard[12] ;
	unsigned char cbSingleLineCount=0 ;
	unsigned char cbLeftCardCount = cbHandCardCount ;
	bool bFindSingleLine = true ;

	//连牌判断
	while (cbLeftCardCount>=5 && bFindSingleLine)
	{
		cbSingleLineCount=1 ;
		bFindSingleLine = false ;
		unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
		cbSingleLineCard[cbSingleLineCount-1] = cbTmpCard[cbFirstCard] ;
		for (unsigned char i=cbFirstCard+1; i<cbLeftCardCount; i++)
		{
			unsigned char cbCardData=cbTmpCard[i];

			//连续判断
			if (1!=(GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbCardData)) && GetCardValue(cbLastCard)!=GetCardValue(cbCardData)) 
			{
				cbLastCard = cbTmpCard[i] ;
				if(cbSingleLineCount<5) 
				{
					cbSingleLineCount = 1 ;
					cbSingleLineCard[cbSingleLineCount-1] = cbTmpCard[i] ;
					continue ;
				}
				else break ;
			}
			//同牌判断
			else if(GetCardValue(cbLastCard)!=GetCardValue(cbCardData))
			{
				cbLastCard = cbCardData ;
				cbSingleLineCard[cbSingleLineCount] = cbCardData ;
				++cbSingleLineCount ;
			}					
		}

		//保存数据
		if(cbSingleLineCount>=5)
		{
			RemoveCard(cbSingleLineCard, cbSingleLineCount, cbTmpCard, cbLeftCardCount) ;
			memcpy(cbLineCardData+cbLineCardCount , cbSingleLineCard, sizeof(unsigned char)*cbSingleLineCount) ;
			cbLineCardCount += cbSingleLineCount ;
			cbLeftCardCount -= cbSingleLineCount ;
			bFindSingleLine = true ;
		}
	}
}

//分析三条
void CAndroidAI::GetAllThreeCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbThreeCardData[], unsigned char &cbThreeCardCount)
{
	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//大小排序
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbThreeCardCount = 0 ;

	//扑克分析
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//变量定义
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//搜索同牌
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}

		if(cbSameCount>=3)
		{
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i] ;
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i+1] ;
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i+2] ;	
		}

		//设置索引
		i+=cbSameCount-1;
	}
}

//分析对子
void CAndroidAI::GetAllDoubleCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbDoubleCardData[], unsigned char &cbDoubleCardCount)
{
	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//大小排序
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbDoubleCardCount = 0 ;

	//扑克分析
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//变量定义
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//搜索同牌
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}

		if(cbSameCount>=2)
		{
			cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i] ;
			cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i+1] ;
		}

		//设置索引
		i+=cbSameCount-1;
	}
}

//分析单牌
void CAndroidAI::GetAllSingleCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbSingleCardData[], unsigned char &cbSingleCardCount)
{
	cbSingleCardCount =0 ;

	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//大小排序
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	//扑克分析
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//变量定义
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//搜索同牌
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}

		if(cbSameCount==1)
		{
			cbSingleCardData[cbSingleCardCount++] = cbTmpCardData[i] ;
		}

		//设置索引
		i+=cbSameCount-1;
	}
}

//分析出牌
void CAndroidAI::AnalyseOutCardType(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12+1])
{
	my_memset(CardTypeResult, sizeof(CardTypeResult[0])*12) ;
	unsigned char cbTmpCardData[MAX_COUNT] ;
	//保留扑克，防止分析时改变扑克
	unsigned char cbReserveCardData[MAX_COUNT] ;
	memcpy(cbReserveCardData, cbHandCardData, cbHandCardCount) ;	
	SortCardList(cbReserveCardData, cbHandCardCount, ST_ORDER) ;
	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	//单牌类型
	for(unsigned char i=0; i<cbHandCardCount; ++i)
	{
		unsigned char Index = CardTypeResult[CT_SINGLE].cbCardTypeCount ;
		CardTypeResult[CT_SINGLE].cbCardType = CT_SINGLE ;
		CardTypeResult[CT_SINGLE].cbCardData[Index][0] = cbTmpCardData[i] ;
		CardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] = 1 ;
		CardTypeResult[CT_SINGLE].cbCardTypeCount++ ;	

		ASSERT(CardTypeResult[CT_SINGLE].cbCardTypeCount<MAX_TYPE_COUNT) ;
	}

	//对牌类型
	{
		unsigned char cbDoubleCardData[MAX_COUNT] ;
		unsigned char cbDoubleCardcount=0; 
		GetAllDoubleCard(cbTmpCardData, cbHandCardCount, cbDoubleCardData, cbDoubleCardcount) ;
		for(unsigned char i=0; i<cbDoubleCardcount; i+=2)
		{
			unsigned char Index = CardTypeResult[CT_DOUBLE].cbCardTypeCount ;
			CardTypeResult[CT_DOUBLE].cbCardType = CT_DOUBLE ;
			CardTypeResult[CT_DOUBLE].cbCardData[Index][0] = cbDoubleCardData[i] ;
			CardTypeResult[CT_DOUBLE].cbCardData[Index][1] = cbDoubleCardData[i+1] ;
			CardTypeResult[CT_DOUBLE].cbEachHandCardCount[Index] = 2 ;
			CardTypeResult[CT_DOUBLE].cbCardTypeCount++ ;	

			ASSERT(CardTypeResult[CT_DOUBLE].cbCardTypeCount<MAX_TYPE_COUNT) ;
		}
	}

	//三条类型
	{
		unsigned char cbThreeCardData[MAX_COUNT];
		unsigned char cbThreeCardCount=0 ;
		GetAllThreeCard(cbTmpCardData, cbHandCardCount, cbThreeCardData, cbThreeCardCount) ;
		for(unsigned char i=0; i<cbThreeCardCount; i+=3)
		{
			unsigned char Index = CardTypeResult[CT_THREE].cbCardTypeCount ;
			CardTypeResult[CT_THREE].cbCardType = CT_THREE ;
			CardTypeResult[CT_THREE].cbCardData[Index][0] = cbThreeCardData[i] ;
			CardTypeResult[CT_THREE].cbCardData[Index][1] = cbThreeCardData[i+1] ;
			CardTypeResult[CT_THREE].cbCardData[Index][2] = cbThreeCardData[i+2] ;
			CardTypeResult[CT_THREE].cbEachHandCardCount[Index] = 3 ;
			CardTypeResult[CT_THREE].cbCardTypeCount++ ;	

			ASSERT(CardTypeResult[CT_THREE].cbCardTypeCount<MAX_TYPE_COUNT) ;
		}
	}

	//炸弹类型
	{
		unsigned char cbFourCardData[MAX_COUNT];
		unsigned char cbFourCardCount=0 ;
		if(cbHandCardCount>=2 && 0x4F==cbTmpCardData[0] && 0x4E==cbTmpCardData[1])
		{
			unsigned char Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount ;
			CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbTmpCardData[0] ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbTmpCardData[1] ;
			CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 2 ;
			CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++ ;	
			GetAllBomCard(cbTmpCardData+2, cbHandCardCount-2, cbFourCardData, cbFourCardCount) ;
		}
		else GetAllBomCard(cbTmpCardData, cbHandCardCount, cbFourCardData, cbFourCardCount) ;
		for (unsigned char i=0; i<cbFourCardCount; i+=4)
		{
			unsigned char Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount ;
			CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbFourCardData[i] ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbFourCardData[i+1] ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][2] = cbFourCardData[i+2] ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][3] = cbFourCardData[i+3] ;
			CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4 ;
			CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++ ;	

			ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount<MAX_TYPE_COUNT) ;
		}
	}
	//单连类型
	{
		//恢复扑克，防止分析时改变扑克
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		unsigned char cbFirstCard = 0 ;
		//去除2和王
		for(unsigned char i=0 ; i<cbHandCardCount ; ++i)
		{
			if(GetCardLogicValue(cbTmpCardData[i])<15)
			{
				cbFirstCard = i ;
				break ;
			}
		}

		unsigned char cbSingleLineCard[12] ;
		unsigned char cbSingleLineCount=1 ;
		unsigned char cbLeftCardCount = cbHandCardCount ;
		bool bFindSingleLine = true ;

		//连牌判断
		while (cbLeftCardCount>=5 && bFindSingleLine)
		{
			cbSingleLineCount=1 ;
			bFindSingleLine = false ;
			unsigned char cbLastCard = cbTmpCardData[cbFirstCard] ;
			cbSingleLineCard[cbSingleLineCount-1] = cbTmpCardData[cbFirstCard] ;
			for (unsigned char i=cbFirstCard+1; i<cbLeftCardCount; i++)
			{
				unsigned char cbCardData=cbTmpCardData[i];

				//连续判断
				if (1!=(GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbCardData)) && GetCardValue(cbLastCard)!=GetCardValue(cbCardData)) 
				{
					cbLastCard = cbTmpCardData[i] ;
					//是否合法
					if(cbSingleLineCount<5) 
					{
						cbSingleLineCount = 1 ;
						cbSingleLineCard[cbSingleLineCount-1] = cbTmpCardData[i] ;
						continue ;
					}
					else break ;
				}
				//同牌判断
				else if(GetCardValue(cbLastCard)!=GetCardValue(cbCardData))
				{
					cbLastCard = cbCardData ;
					cbSingleLineCard[cbSingleLineCount] = cbCardData ;
					++cbSingleLineCount ;
				}
			}

			//保存数据
			if(cbSingleLineCount>=5)
			{
				unsigned char Index ;
				//所有连牌
				unsigned char cbStart=0 ;
				//从大到小
				while (cbSingleLineCount-cbStart>=5)
				{
					Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount ;
					unsigned char cbThisLineCount = cbSingleLineCount-cbStart ;
					CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE ;
					memcpy(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard+cbStart, sizeof(unsigned char)*(cbThisLineCount));
					CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbThisLineCount;
					CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount<MAX_TYPE_COUNT) ;
					cbStart++ ;
				}
				//从小到大
				cbStart=1 ;
				while (cbSingleLineCount-cbStart>=5)
				{
					Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount ;
					unsigned char cbThisLineCount = cbSingleLineCount-cbStart ;
					CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE ;
					memcpy(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard, sizeof(unsigned char)*(cbThisLineCount));
					CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbThisLineCount;
					CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount<MAX_TYPE_COUNT) ;
					cbStart++ ;
				}

				RemoveCard(cbSingleLineCard, cbSingleLineCount, cbTmpCardData, cbLeftCardCount) ;
				cbLeftCardCount -= cbSingleLineCount ;
				bFindSingleLine = true ;
			}
		}

	}

	//对连类型
	{
		//恢复扑克，防止分析时改变扑克
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		//连牌判断
		unsigned char cbFirstCard = 0 ;
		//去除2和王
		for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCardData[i])<15)	{cbFirstCard = i ; break ;}

		unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
		bool bFindDoubleLine = true ;
		unsigned char cbDoubleLineCount = 0 ;
		unsigned char cbDoubleLineCard[24] ;
		//开始判断
		while (cbLeftCardCount>=6 && bFindDoubleLine)
		{
			unsigned char cbLastCard = cbTmpCardData[cbFirstCard] ;
			unsigned char cbSameCount = 1 ;
			cbDoubleLineCount = 0 ;
			bFindDoubleLine=false ;
			for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
			{
				//搜索同牌
				while (GetCardLogicValue(cbLastCard)==GetCardLogicValue(cbTmpCardData[i]) && i<cbLeftCardCount+cbFirstCard)
				{
					++cbSameCount;
					++i ;
				}

				unsigned char cbLastDoubleCardValue ;
				if(cbDoubleLineCount>0) cbLastDoubleCardValue = GetCardLogicValue(cbDoubleLineCard[cbDoubleLineCount-1]) ;
				//重新开始
				if((cbSameCount<2 || (cbDoubleLineCount>0 && (cbLastDoubleCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
				{
					if(cbDoubleLineCount>=6) break ;
					//回退
					if(cbSameCount>=2) i-=cbSameCount ;
					cbLastCard = cbTmpCardData[i] ;
					cbDoubleLineCount = 0 ;
				}
				//保存数据
				else if(cbSameCount>=2)
				{
					cbDoubleLineCard[cbDoubleLineCount] = cbTmpCardData[i-cbSameCount] ;
					cbDoubleLineCard[cbDoubleLineCount+1] = cbTmpCardData[i-cbSameCount+1] ;
					cbDoubleLineCount += 2 ;

					//结尾判断
					if(i==(cbLeftCardCount+cbFirstCard-2))
						if((GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbTmpCardData[i]))==1 && (GetCardLogicValue(cbTmpCardData[i])==GetCardLogicValue(cbTmpCardData[i+1])))
						{
							cbDoubleLineCard[cbDoubleLineCount] = cbTmpCardData[i] ;
							cbDoubleLineCard[cbDoubleLineCount+1] = cbTmpCardData[i+1] ;
							cbDoubleLineCount += 2 ;
							break ;
						}

				}

				cbLastCard = cbTmpCardData[i] ;
				cbSameCount = 1 ;
			}

			//保存数据
			if(cbDoubleLineCount>=6)
			{
				unsigned char Index ;

				Index = CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount ;
				CardTypeResult[CT_DOUBLE_LINE].cbCardType = CT_DOUBLE_LINE ;
				memcpy(CardTypeResult[CT_DOUBLE_LINE].cbCardData[Index], cbDoubleLineCard, sizeof(unsigned char)*cbDoubleLineCount);
				CardTypeResult[CT_DOUBLE_LINE].cbEachHandCardCount[Index] = cbDoubleLineCount;
				CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount++ ;

				ASSERT(CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount<MAX_TYPE_COUNT) ;

				RemoveCard(cbDoubleLineCard, cbDoubleLineCount, cbTmpCardData, cbFirstCard+cbLeftCardCount) ;				
				bFindDoubleLine=true ;
				cbLeftCardCount -= cbDoubleLineCount ;
			}
		}
	}

	//三连类型
	{
		//恢复扑克，防止分析时改变扑克
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		//连牌判断
		unsigned char cbFirstCard = 0 ;
		//去除2和王
		for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCardData[i])<15)	{cbFirstCard = i ; break ;}

		unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
		bool bFindThreeLine = true ;
		unsigned char cbThreeLineCount = 0 ;
		unsigned char cbThreeLineCard[20] ;
		//开始判断
		while (cbLeftCardCount>=6 && bFindThreeLine)
		{
			unsigned char cbLastCard = cbTmpCardData[cbFirstCard] ;
			unsigned char cbSameCount = 1 ;
			cbThreeLineCount = 0 ;
			bFindThreeLine = false ;
			for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
			{
				//搜索同牌
				while (GetCardLogicValue(cbLastCard)==GetCardLogicValue(cbTmpCardData[i]) && i<cbLeftCardCount+cbFirstCard)
				{
					++cbSameCount;
					++i ;
				}

				unsigned char cbLastThreeCardValue ;
				if(cbThreeLineCount>0) cbLastThreeCardValue = GetCardLogicValue(cbThreeLineCard[cbThreeLineCount-1]) ;

				//重新开始
				if((cbSameCount<3 || (cbThreeLineCount>0&&(cbLastThreeCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
				{
					if(cbThreeLineCount>=6) break ;

					if(cbSameCount>=3) i-=cbSameCount ;
					cbLastCard = cbTmpCardData[i] ;
					cbThreeLineCount = 0 ;
				}
				//保存数据
				else if(cbSameCount>=3)
				{
					cbThreeLineCard[cbThreeLineCount] = cbTmpCardData[i-cbSameCount] ;
					cbThreeLineCard[cbThreeLineCount+1] = cbTmpCardData[i-cbSameCount+1] ;
					cbThreeLineCard[cbThreeLineCount+2] = cbTmpCardData[i-cbSameCount+2] ;
					cbThreeLineCount += 3 ;

					//结尾判断
					if(i==(cbLeftCardCount+cbFirstCard-3))
						if((GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbTmpCardData[i]))==1 && (GetCardLogicValue(cbTmpCardData[i])==GetCardLogicValue(cbTmpCardData[i+1])) && (GetCardLogicValue(cbTmpCardData[i])==GetCardLogicValue(cbTmpCardData[i+2])))
						{
							cbThreeLineCard[cbThreeLineCount] = cbTmpCardData[i] ;
							cbThreeLineCard[cbThreeLineCount+1] = cbTmpCardData[i+1] ;
							cbThreeLineCard[cbThreeLineCount+2] = cbTmpCardData[i+2] ;
							cbThreeLineCount += 3 ;
							break ;
						}

				}

				cbLastCard = cbTmpCardData[i] ;
				cbSameCount = 1 ;
			}

			//保存数据
			if(cbThreeLineCount>=6)
			{
				unsigned char Index ;

				Index = CardTypeResult[CT_THREE_LINE].cbCardTypeCount ;
				CardTypeResult[CT_THREE_LINE].cbCardType = CT_THREE_LINE ;
				memcpy(CardTypeResult[CT_THREE_LINE].cbCardData[Index], cbThreeLineCard, sizeof(unsigned char)*cbThreeLineCount);
				CardTypeResult[CT_THREE_LINE].cbEachHandCardCount[Index] = cbThreeLineCount;
				CardTypeResult[CT_THREE_LINE].cbCardTypeCount++ ;

				ASSERT(CardTypeResult[CT_THREE_LINE].cbCardTypeCount<MAX_TYPE_COUNT) ;

				RemoveCard(cbThreeLineCard, cbThreeLineCount, cbTmpCardData, cbFirstCard+cbLeftCardCount) ;
				bFindThreeLine=true ;
				cbLeftCardCount -= cbThreeLineCount ;
			}
		}

	}
	//三带一单
	{
		//恢复扑克，防止分析时改变扑克
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		unsigned char cbHandThreeCard[MAX_COUNT];
		unsigned char cbHandThreeCount=0 ;

		//移除炸弹
		unsigned char cbAllBomCardData[MAX_COUNT] ;
		unsigned char cbAllBomCardCount=0 ;
		GetAllBomCard(cbTmpCardData, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount) ;
		RemoveCard(cbAllBomCardData, cbAllBomCardCount, cbTmpCardData, cbHandCardCount) ;

		GetAllThreeCard(cbTmpCardData, cbHandCardCount-cbAllBomCardCount, cbHandThreeCard, cbHandThreeCount) ;

		{
			unsigned char Index ;
			//去掉三条
			unsigned char cbRemainCardData[MAX_COUNT] ;
			memcpy(cbRemainCardData, cbTmpCardData, cbHandCardCount-cbAllBomCardCount) ;
			unsigned char cbRemainCardCount=cbHandCardCount-cbAllBomCardCount-cbHandThreeCount ;
			RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCardData, cbHandCardCount-cbAllBomCardCount) ;
			//三条带一张
			for(unsigned char i=0; i<cbHandThreeCount; i+=3)
			{
				//三条带一张
				for(unsigned char j=0; j<cbRemainCardCount; ++j)
				{
					Index = CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount ;
					CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardType = CT_THREE_LINE_TAKE_ONE ;
					CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index][0] = cbHandThreeCard[i] ;
					CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index][1] = cbHandThreeCard[i+1] ;
					CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index][2] = cbHandThreeCard[i+2] ;
					CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index][3] = cbRemainCardData[j] ;
					CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbEachHandCardCount[Index] = 4 ;
					CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount++ ;
				}			
			}
		}

		//三连带单
		unsigned char cbLeftThreeCardCount=cbHandThreeCount ;
		bool bFindThreeLine=true ;
		unsigned char cbLastIndex=0 ;
		if(GetCardLogicValue(cbHandThreeCard[0])==15) cbLastIndex=3 ;
		while (cbLeftThreeCardCount>=6 && bFindThreeLine)
		{
			unsigned char cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[cbLastIndex]);
			unsigned char cbThreeLineCard[MAX_COUNT];
			unsigned char cbThreeLineCardCount=3;
			cbThreeLineCard[0]=cbHandThreeCard[cbLastIndex];
			cbThreeLineCard[1]=cbHandThreeCard[cbLastIndex+1];
			cbThreeLineCard[2]=cbHandThreeCard[cbLastIndex+2];

			bFindThreeLine = false ;
			for(unsigned char j=3+cbLastIndex; j<cbLeftThreeCardCount; j+=3)
			{
				//连续判断
				if(1!=(cbLastLogicCard-(GetCardLogicValue(cbHandThreeCard[j]))))
				{
					cbLastIndex = j ;
					if(cbLeftThreeCardCount-j>=6) bFindThreeLine = true ;

					break;
				}

				cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[j]);
				cbThreeLineCard[cbThreeLineCardCount]=cbHandThreeCard[j];
				cbThreeLineCard[cbThreeLineCardCount+1]=cbHandThreeCard[j+1];
				cbThreeLineCard[cbThreeLineCardCount+2]=cbHandThreeCard[j+2];
				cbThreeLineCardCount += 3;
			}
			if(cbThreeLineCardCount>3)
			{
				unsigned char Index ;

				unsigned char cbRemainCard[MAX_COUNT];
				unsigned char cbRemainCardCount=cbHandCardCount-cbAllBomCardCount-cbHandThreeCount ;


				//移除三条（还应该移除炸弹王等）
				memcpy(cbRemainCard, cbTmpCardData, (cbHandCardCount-cbAllBomCardCount)*sizeof(unsigned char));
				RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCard, cbHandCardCount-cbAllBomCardCount) ;

				for(unsigned char start=0; start<cbThreeLineCardCount-3; start+=3)
				{
					//本顺数目
					unsigned char cbThisTreeLineCardCount = cbThreeLineCardCount-start ;
					//单牌个数
					unsigned char cbSingleCardCount=(cbThisTreeLineCardCount)/3;

					//单牌不够
					if(cbRemainCardCount<cbSingleCardCount) continue ;

					//单牌组合
					unsigned char cbComCard[5];
					unsigned char cbComResCard[254][5] ;
					unsigned char cbComResLen=0 ;

					Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, cbSingleCardCount, cbRemainCardCount, cbSingleCardCount);
					for(unsigned char i=0; i<cbComResLen; ++i)
					{
						Index = CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount ;
						CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardType = CT_THREE_LINE_TAKE_ONE ;
						//保存三条
						memcpy(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index], cbThreeLineCard+start, sizeof(unsigned char)*cbThisTreeLineCardCount);
						//保存单牌
						memcpy(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index]+cbThisTreeLineCardCount, cbComResCard[i], cbSingleCardCount) ;


						CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbEachHandCardCount[Index] = cbThisTreeLineCardCount+cbSingleCardCount ;
						CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount++ ;

						ASSERT(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount<MAX_TYPE_COUNT) ;
					}

				}

				//移除三连
				bFindThreeLine = true ;
				RemoveCard(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount) ;
				cbLeftThreeCardCount -= cbThreeLineCardCount ;
			}
		}
	}

	//三带一对
	{
		//恢复扑克，防止分析时改变扑克
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		unsigned char cbHandThreeCard[MAX_COUNT];
		unsigned char cbHandThreeCount=0 ;
		unsigned char cbRemainCarData[MAX_COUNT] ;
		unsigned char cbRemainCardCount=0 ;

		//抽取三条
		GetAllThreeCard(cbTmpCardData, cbHandCardCount, cbHandThreeCard, cbHandThreeCount) ;

		//移除三条（还应该移除炸弹王等）
		memcpy(cbRemainCarData, cbTmpCardData, cbHandCardCount) ;
		RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCarData, cbHandCardCount) ;
		cbRemainCardCount = cbHandCardCount-cbHandThreeCount ;

		//抽取对牌
		unsigned char cbAllDoubleCardData[MAX_COUNT] ;
		unsigned char cbAllDoubleCardCount=0 ;
		GetAllDoubleCard(cbRemainCarData, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;

		//三条带一对
		for(unsigned char i=0; i<cbHandThreeCount; i+=3)
		{
			unsigned char Index ;

			//三条带一张
			for(unsigned char j=0; j<cbAllDoubleCardCount; j+=2)
			{
				Index = CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardType = CT_THREE_LINE_TAKE_TWO ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][0] = cbHandThreeCard[i] ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][1] = cbHandThreeCard[i+1] ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][2] = cbHandThreeCard[i+2] ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][3] = cbAllDoubleCardData[j] ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][4] = cbAllDoubleCardData[j+1] ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbEachHandCardCount[Index] = 5 ;
				CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount++ ;
			}	
		}

		//三连带对
		unsigned char cbLeftThreeCardCount=cbHandThreeCount ;
		bool bFindThreeLine=true ;
		unsigned char cbLastIndex=0 ;
		if(GetCardLogicValue(cbHandThreeCard[0])==15) cbLastIndex=3 ;
		while (cbLeftThreeCardCount>=6 && bFindThreeLine)
		{
			unsigned char cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[cbLastIndex]);
			unsigned char cbThreeLineCard[MAX_COUNT];
			unsigned char cbThreeLineCardCount=3;
			cbThreeLineCard[0]=cbHandThreeCard[cbLastIndex];
			cbThreeLineCard[1]=cbHandThreeCard[cbLastIndex+1];
			cbThreeLineCard[2]=cbHandThreeCard[cbLastIndex+2];

			bFindThreeLine=false ;
			for(unsigned char j=3+cbLastIndex; j<cbLeftThreeCardCount; j+=3)
			{
				//连续判断
				if(1!=(cbLastLogicCard-(GetCardLogicValue(cbHandThreeCard[j]))))
				{
					cbLastIndex = j ;
					if(cbLeftThreeCardCount-j>=6) bFindThreeLine = true ;

					break;
				}

				cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[j]);
				cbThreeLineCard[cbThreeLineCardCount]=cbHandThreeCard[j];
				cbThreeLineCard[cbThreeLineCardCount+1]=cbHandThreeCard[j+1];
				cbThreeLineCard[cbThreeLineCardCount+2]=cbHandThreeCard[j+2];
				cbThreeLineCardCount += 3;
			}
			if(cbThreeLineCardCount>3)
			{
				unsigned char Index ;

				for(unsigned char start=0; start<cbThreeLineCardCount-3; start+=3)
				{
					//本顺数目
					unsigned char cbThisTreeLineCardCount = cbThreeLineCardCount-start ;
					//对牌张数
					unsigned char cbDoubleCardCount=((cbThisTreeLineCardCount)/3);

					//对牌不够
					if(cbRemainCardCount<cbDoubleCardCount) continue ;

					unsigned char cbDoubleCardIndex[10]; //对牌下标
					for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
						cbDoubleCardIndex[j]=i ;

					//对牌组合
					unsigned char cbComCard[5];
					unsigned char cbComResCard[254][5] ;
					unsigned char cbComResLen=0 ;

					//利用对牌的下标做组合，再根据下标提取出对牌
					Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, cbDoubleCardCount, cbAllDoubleCardCount/2, cbDoubleCardCount);

					ASSERT(cbComResLen<=254) ;

					for(unsigned char i=0; i<cbComResLen; ++i)
					{
						Index = CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount ;
						CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardType = CT_THREE_LINE_TAKE_TWO ;
						//保存三条
						memcpy(CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index], cbThreeLineCard+start, sizeof(unsigned char)*cbThisTreeLineCardCount);
						//保存对牌
						for(unsigned char j=0, k=0; j<cbDoubleCardCount; ++j, k+=2)
						{
							CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][cbThisTreeLineCardCount+k] = cbAllDoubleCardData[cbComResCard[i][j]];
							CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][cbThisTreeLineCardCount+k+1] = cbAllDoubleCardData[cbComResCard[i][j]+1];
						}

						CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbEachHandCardCount[Index] = cbThisTreeLineCardCount+2*cbDoubleCardCount ;
						CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount++ ;

						ASSERT(CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount<MAX_TYPE_COUNT) ;
					}

				}
				//移除三连
				bFindThreeLine = true ;				
				RemoveCard(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount) ;
				cbLeftThreeCardCount -= cbThreeLineCardCount ;
			}
		}
	}
	//四带两单
	/*
	{
	//恢复扑克，防止分析时改变扑克
	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	unsigned char cbFirstCard = 0 ;
	//去除王牌
	for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

	unsigned char cbHandAllFourCardData[MAX_COUNT] ;
	unsigned char cbHandAllFourCardCount=0;
	//抽取四张
	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

	//移除四条
	unsigned char cbRemainCard[MAX_COUNT];
	unsigned char cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
	memcpy(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(unsigned char));
	RemoveCard(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

	for(unsigned char Start=0; Start<cbHandAllFourCardCount; Start += 4)
	{
	unsigned char Index ;
	//单牌组合
	unsigned char cbComCard[5];
	unsigned char cbComResCard[254][5] ;
	unsigned char cbComResLen=0 ;
	//单牌组合
	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, 2, cbRemainCardCount, 2);
	for(unsigned char i=0; i<cbComResLen; ++i)
	{
	//不能带对
	if(GetCardValue(cbComResCard[i][0])==GetCardValue(cbComResCard[i][1])) continue ;

	Index=CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardTypeCount ;
	CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardType = CT_FOUR_LINE_TAKE_ONE ;
	memcpy(CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardData[Index], cbHandAllFourCardData+Start, 4) ;
	CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardData[Index][4] = cbComResCard[i][0] ;
	CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardData[Index][4+1] = cbComResCard[i][1] ;
	CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbEachHandCardCount[Index] = 6 ;
	CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardTypeCount++ ;

	ASSERT(CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardTypeCount<MAX_TYPE_COUNT) ;
	}
	}
	}*/


	//四带两对
	/*
	{
	//恢复扑克，防止分析时改变扑克
	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	unsigned char cbFirstCard = 0 ;
	//去除王牌
	for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

	unsigned char cbHandAllFourCardData[MAX_COUNT] ;
	unsigned char cbHandAllFourCardCount=0;

	//抽取四张
	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

	//移除四条
	unsigned char cbRemainCard[MAX_COUNT];
	unsigned char cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
	memcpy(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(unsigned char));
	RemoveCard(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

	for(unsigned char Start=0; Start<cbHandAllFourCardCount; Start += 4)
	{
	//抽取对牌
	unsigned char cbAllDoubleCardData[MAX_COUNT] ;
	unsigned char cbAllDoubleCardCount=0 ;
	GetAllDoubleCard(cbRemainCard, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;

	unsigned char cbDoubleCardIndex[10]; //对牌下标
	for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
	cbDoubleCardIndex[j]=i ;

	//对牌组合
	unsigned char cbComCard[5];
	unsigned char cbComResCard[255][5] ;
	unsigned char cbComResLen=0 ;

	//利用对牌的下标做组合，再根据下标提取出对牌
	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, 2, cbAllDoubleCardCount/2, 2);
	for(unsigned char i=0; i<cbComResLen; ++i)
	{
	unsigned char Index = CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount ;
	CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardType = CT_FOUR_LINE_TAKE_TWO ;
	memcpy(CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index], cbHandAllFourCardData+Start, 4) ;

	//保存对牌
	for(unsigned char j=0, k=0; j<4; ++j, k+=2)
	{
	CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index][4+k] = cbAllDoubleCardData[cbComResCard[i][j]];
	CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index][4+k+1] = cbAllDoubleCardData[cbComResCard[i][j]+1];
	}

	CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbEachHandCardCount[Index] = 8 ;
	CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount++ ;

	ASSERT(CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount<MAX_TYPE_COUNT) ;
	}
	}
	}*/

}

//分析牌型
void CAndroidAI::AnalyseOutCardType(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char const cbTurnCardData[], unsigned char const cbTurnCardCount,tagOutCardTypeResult CardTypeResult[12+1])
{
	my_memset(CardTypeResult, sizeof(CardTypeResult[0])*12) ;
	//数据校验
	if(cbHandCardCount<cbTurnCardCount) return ;

	unsigned char cbTmpCard[MAX_COUNT];
	memcpy(cbTmpCard, cbHandCardData, cbHandCardCount) ;
	SortCardList(cbTmpCard, cbHandCardCount, ST_ORDER) ;
	//	SortCardList(cbTurnCardData, cbTurnCardCount, ST_ORDER) ;

	unsigned char cbTurnCardType = GetCardType(cbTurnCardData, cbTurnCardCount) ;
	ASSERT(cbTurnCardType!=CT_ERROR) ;
	if(cbTurnCardType==CT_ERROR)
		return ;

	if(cbTurnCardType!=CT_MISSILE_CARD && cbTurnCardType!=CT_BOMB_CARD)
	{
		//双王炸弹
		if(cbHandCardCount>=2 && 0x4F==cbTmpCard[0] && 0x4E==cbTmpCard[1])
		{
			unsigned char Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
			CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbTmpCard[0] ;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbTmpCard[1] ;
			CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 2 ;
			CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

			unsigned char cbBomCardData[MAX_COUNT];
			unsigned char cbBomCardCount=0;
			GetAllBomCard(cbTmpCard+2, cbHandCardCount-2, cbBomCardData, cbBomCardCount) ;
			for(unsigned char i=0; i<cbBomCardCount/4; ++i)
			{
				Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
				CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD ;
				memcpy(CardTypeResult[CT_BOMB_CARD].cbCardData[Index], cbBomCardData+4*i, 4) ;
				CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4;
				CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

				ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount<=MAX_TYPE_COUNT) ;
			}
		}
		//炸弹牌型
		else
		{
			unsigned char cbBomCardData[MAX_COUNT];
			unsigned char cbBomCardCount=0;
			GetAllBomCard(cbTmpCard, cbHandCardCount, cbBomCardData, cbBomCardCount) ;
			for(unsigned char i=0; i<cbBomCardCount/4; ++i)
			{
				unsigned char Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
				CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD ;
				memcpy(CardTypeResult[CT_BOMB_CARD].cbCardData[Index], cbBomCardData+4*i, 4) ;
				CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4;
				CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

				ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount<=MAX_TYPE_COUNT) ;
			}
		}
	}

	switch(cbTurnCardType)
	{
	case CT_SINGLE:				//单牌类型
		{			
			for(unsigned char i=0; i<cbHandCardCount; ++i) 
				if(GetCardLogicValue(cbTmpCard[i])>GetCardLogicValue(cbTurnCardData[0])) 
				{
					unsigned char Index = CardTypeResult[CT_SINGLE].cbCardTypeCount ;
					CardTypeResult[CT_SINGLE].cbCardType = CT_SINGLE ;
					CardTypeResult[CT_SINGLE].cbCardData[Index][0] = cbTmpCard[i];
					CardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] = 1;
					CardTypeResult[CT_SINGLE].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_SINGLE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
				}
				break ;
		}
	case CT_DOUBLE:				//对牌类型
		{
			//扑克分析
			for (unsigned char i=0;i<cbHandCardCount;i++)
			{
				//变量定义
				unsigned char cbSameCount=1;
				unsigned char cbLogicValue=GetCardLogicValue(cbTmpCard[i]);

				//搜索同牌
				for (unsigned char j=i+1;j<cbHandCardCount;j++)
				{
					//获取扑克
					if (GetCardLogicValue(cbTmpCard[j])!=cbLogicValue) break;

					//设置变量
					cbSameCount++;
				}

				if(cbSameCount>=2 && GetCardLogicValue(cbTmpCard[i])>GetCardLogicValue(cbTurnCardData[0]))
				{
					unsigned char Index = CardTypeResult[CT_DOUBLE].cbCardTypeCount ;
					CardTypeResult[CT_DOUBLE].cbCardType = CT_DOUBLE ;
					CardTypeResult[CT_DOUBLE].cbCardData[Index][0] = cbTmpCard[i];
					CardTypeResult[CT_DOUBLE].cbCardData[Index][1] = cbTmpCard[i+1];
					CardTypeResult[CT_DOUBLE].cbEachHandCardCount[Index] = 2;
					CardTypeResult[CT_DOUBLE].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_DOUBLE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
				}
				//设置索引
				i+=cbSameCount-1;
			}
			break ;
		}
	case CT_THREE:				//三条类型
		{
			//扑克分析
			for (unsigned char i=0;i<cbHandCardCount;i++)
			{
				//变量定义
				unsigned char cbSameCount=1;
				unsigned char cbLogicValue=GetCardLogicValue(cbTmpCard[i]);

				//搜索同牌
				for (unsigned char j=i+1;j<cbHandCardCount;j++)
				{
					//获取扑克
					if (GetCardLogicValue(cbTmpCard[j])!=cbLogicValue) break;

					//设置变量
					cbSameCount++;
				}

				if(cbSameCount>=3 && GetCardLogicValue(cbTmpCard[i])>GetCardLogicValue(cbTurnCardData[0]))
				{
					unsigned char Index = CardTypeResult[CT_THREE].cbCardTypeCount ;
					CardTypeResult[CT_THREE].cbCardType = CT_THREE ;
					CardTypeResult[CT_THREE].cbCardData[Index][0] = cbTmpCard[i];
					CardTypeResult[CT_THREE].cbCardData[Index][1] = cbTmpCard[i+1];
					CardTypeResult[CT_THREE].cbCardData[Index][2] = cbTmpCard[i+2];
					CardTypeResult[CT_THREE].cbEachHandCardCount[Index] = 3;
					CardTypeResult[CT_THREE].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_THREE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
				}
				//设置索引
				i+=cbSameCount-1;
			}
			break ;
		}
	case CT_SINGLE_LINE:		//单连类型
		{
			unsigned char cbFirstCard = 0 ;
			//去除2和王
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

			unsigned char cbSingleLineCard[12] ;
			unsigned char cbSingleLineCount=1 ;
			unsigned char cbLeftCardCount = cbHandCardCount ;
			bool bFindSingleLine = true ;

			//连牌判断
			while (cbLeftCardCount>=cbTurnCardCount && bFindSingleLine)
			{
				cbSingleLineCount=1 ;
				bFindSingleLine = false ;
				unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
				cbSingleLineCard[cbSingleLineCount-1] = cbTmpCard[cbFirstCard] ;
				for (unsigned char i=cbFirstCard+1; i<cbLeftCardCount; i++)
				{
					unsigned char cbCardData=cbTmpCard[i];

					//连续判断
					if (1!=(GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbCardData)) && GetCardValue(cbLastCard)!=GetCardValue(cbCardData)) 
					{
						cbLastCard = cbTmpCard[i] ;
						//是否合法
						if(cbSingleLineCount<cbTurnCardCount) 
						{
							cbSingleLineCount = 1 ;
							cbSingleLineCard[cbSingleLineCount-1] = cbTmpCard[i] ;
							continue ;
						}
						else break ;
					}
					//同牌判断
					else if(GetCardValue(cbLastCard)!=GetCardValue(cbCardData))
					{
						cbLastCard = cbCardData ;
						cbSingleLineCard[cbSingleLineCount] = cbCardData ;
						++cbSingleLineCount ;
					}
				}

				//保存数据
				if(cbSingleLineCount>=cbTurnCardCount && GetCardLogicValue(cbSingleLineCard[0])>GetCardLogicValue(cbTurnCardData[0]))
				{
					unsigned char Index ;
					unsigned char cbStart=0 ;
					//所有连牌
					while (GetCardLogicValue(cbSingleLineCard[cbStart])>GetCardLogicValue(cbTurnCardData[0]) && ((cbSingleLineCount-cbStart)>=cbTurnCardCount))
					{
						Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount ;
						CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE ;
						memcpy(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard+cbStart, sizeof(unsigned char)*cbTurnCardCount);
						CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbTurnCardCount;
						CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++ ;
						cbStart++;

						ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
					}

					RemoveCard(cbSingleLineCard, cbSingleLineCount, cbTmpCard, cbLeftCardCount) ;
					cbLeftCardCount -= cbSingleLineCount ;
					bFindSingleLine = true ;
				}
			}

			break ;
		}
	case CT_DOUBLE_LINE:		//对连类型
		{
			//连牌判断
			unsigned char cbFirstCard = 0 ;
			//去除2和王
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

			unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
			bool bFindDoubleLine = true ;
			unsigned char cbDoubleLineCount = 0 ;
			unsigned char cbDoubleLineCard[24] ;
			//开始判断
			while (cbLeftCardCount>=cbTurnCardCount && bFindDoubleLine)
			{
				unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
				unsigned char cbSameCount = 1 ;
				cbDoubleLineCount = 0 ;
				bFindDoubleLine=false ;
				for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
				{
					//搜索同牌
					while (GetCardValue(cbLastCard)==GetCardValue(cbTmpCard[i]) && i<cbLeftCardCount+cbFirstCard)
					{
						++cbSameCount;
						++i ;
					}

					unsigned char cbLastDoubleCardValue ;
					if(cbDoubleLineCount>0) cbLastDoubleCardValue = GetCardLogicValue(cbDoubleLineCard[cbDoubleLineCount-1]) ;
					//重新开始
					if((cbSameCount<2 || (cbDoubleLineCount>0 && (cbLastDoubleCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
					{
						if(cbDoubleLineCount>=cbTurnCardCount) break ;

						if(cbSameCount>=2) i-=cbSameCount ;

						cbLastCard = cbTmpCard[i] ;
						cbDoubleLineCount = 0 ;
					}
					//保存数据
					else if(cbSameCount>=2)
					{
						cbDoubleLineCard[cbDoubleLineCount] = cbTmpCard[i-cbSameCount] ;
						cbDoubleLineCard[cbDoubleLineCount+1] = cbTmpCard[i-cbSameCount+1] ;
						cbDoubleLineCount += 2 ;

						//结尾判断
						if(i==(cbLeftCardCount+cbFirstCard-2))
							if((GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbTmpCard[i]))==1 && (GetCardLogicValue(cbTmpCard[i])==GetCardLogicValue(cbTmpCard[i+1])))
							{
								cbDoubleLineCard[cbDoubleLineCount] = cbTmpCard[i] ;
								cbDoubleLineCard[cbDoubleLineCount+1] = cbTmpCard[i+1] ;
								cbDoubleLineCount += 2 ;
								break ;
							}

					}

					cbLastCard = cbTmpCard[i] ;
					cbSameCount = 1 ;
				}

				//保存数据
				if(cbDoubleLineCount>=cbTurnCardCount)
				{
					unsigned char Index ;
					unsigned char cbStart=0 ;
					//所有连牌
					while (GetCardLogicValue(cbDoubleLineCard[cbStart])>GetCardLogicValue(cbTurnCardData[0]) && ((cbDoubleLineCount-cbStart)>=cbTurnCardCount))
					{
						Index = CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount ;
						CardTypeResult[CT_DOUBLE_LINE].cbCardType = CT_DOUBLE_LINE ;
						memcpy(CardTypeResult[CT_DOUBLE_LINE].cbCardData[Index], cbDoubleLineCard+cbStart, sizeof(unsigned char)*cbTurnCardCount);
						CardTypeResult[CT_DOUBLE_LINE].cbEachHandCardCount[Index] = cbTurnCardCount;
						CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount++ ;
						cbStart += 2;

						ASSERT(CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
					}
					RemoveCard(cbDoubleLineCard, cbDoubleLineCount, cbTmpCard, cbFirstCard+cbLeftCardCount) ;				
					bFindDoubleLine=true ;
					cbLeftCardCount -= cbDoubleLineCount ;
				}
			}

			break;
		}
	case CT_THREE_LINE:			//三连类型
		{
			//连牌判断
			unsigned char cbFirstCard = 0 ;
			//去除2和王
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

			unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
			bool bFindThreeLine = true ;
			unsigned char cbThreeLineCount = 0 ;
			unsigned char cbThreeLineCard[20] ;
			//开始判断
			while (cbLeftCardCount>=cbTurnCardCount && bFindThreeLine)
			{
				unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
				unsigned char cbSameCount = 1 ;
				cbThreeLineCount = 0 ;
				bFindThreeLine = false ;
				for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
				{
					//搜索同牌
					while (GetCardValue(cbLastCard)==GetCardValue(cbTmpCard[i]) && i<cbLeftCardCount+cbFirstCard)
					{
						++cbSameCount;
						++i ;
					}

					unsigned char cbLastThreeCardValue ;
					if(cbThreeLineCount>0) cbLastThreeCardValue = GetCardLogicValue(cbThreeLineCard[cbThreeLineCount-1]) ;

					//重新开始
					if((cbSameCount<3 || (cbThreeLineCount>0&&(cbLastThreeCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
					{
						if(cbThreeLineCount>=cbTurnCardCount) break ;

						if(cbSameCount>=3) i-= 3 ;
						cbLastCard = cbTmpCard[i] ;
						cbThreeLineCount = 0 ;
					}
					//保存数据
					else if(cbSameCount>=3)
					{
						cbThreeLineCard[cbThreeLineCount] = cbTmpCard[i-cbSameCount] ;
						cbThreeLineCard[cbThreeLineCount+1] = cbTmpCard[i-cbSameCount+1] ;
						cbThreeLineCard[cbThreeLineCount+2] = cbTmpCard[i-cbSameCount+2] ;
						cbThreeLineCount += 3 ;

						//结尾判断
						if(i==(cbLeftCardCount+cbFirstCard-3))
							if((GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbTmpCard[i]))==1 && (GetCardLogicValue(cbTmpCard[i])==GetCardLogicValue(cbTmpCard[i+1])) && (GetCardLogicValue(cbTmpCard[i])==GetCardLogicValue(cbTmpCard[i+2])))
							{
								cbThreeLineCard[cbThreeLineCount] = cbTmpCard[i] ;
								cbThreeLineCard[cbThreeLineCount+1] = cbTmpCard[i+1] ;
								cbThreeLineCard[cbThreeLineCount+2] = cbTmpCard[i+2] ;
								cbThreeLineCount += 3 ;
								break ;
							}

					}

					cbLastCard = cbTmpCard[i] ;
					cbSameCount = 1 ;
				}

				//保存数据
				if(cbThreeLineCount>=cbTurnCardCount)
				{
					unsigned char Index ;
					unsigned char cbStart=0 ;
					//所有连牌
					while (GetCardLogicValue(cbThreeLineCard[cbStart])>GetCardLogicValue(cbTurnCardData[0]) && ((cbThreeLineCount-cbStart)>=cbTurnCardCount))
					{
						Index = CardTypeResult[CT_THREE_LINE].cbCardTypeCount ;
						CardTypeResult[CT_THREE_LINE].cbCardType = CT_THREE_LINE ;
						memcpy(CardTypeResult[CT_THREE_LINE].cbCardData[Index], cbThreeLineCard+cbStart, sizeof(unsigned char)*cbTurnCardCount);
						CardTypeResult[CT_THREE_LINE].cbEachHandCardCount[Index] = cbTurnCardCount;
						CardTypeResult[CT_THREE_LINE].cbCardTypeCount++ ;
						cbStart += 3;

						ASSERT(CardTypeResult[CT_THREE_LINE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
					}

					RemoveCard(cbThreeLineCard, cbThreeLineCount, cbTmpCard, cbFirstCard+cbLeftCardCount) ;
					bFindThreeLine=true ;
					cbLeftCardCount -= cbThreeLineCount ;
				}
			}

			break;
		}
	case CT_THREE_LINE_TAKE_ONE://三带一单
		{
			unsigned char cbTurnThreeCard[MAX_COUNT]; 
			unsigned char cbTurnThreeCount=0;
			unsigned char cbHandThreeCard[MAX_COUNT];
			unsigned char cbHandThreeCount=0 ;
			unsigned char cbSingleCardCount=cbTurnCardCount/4;

			//移除炸弹
			unsigned char cbAllBomCardData[MAX_COUNT] ;
			unsigned char cbAllBomCardCount=0 ;
			GetAllBomCard(cbTmpCard, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount) ;
			RemoveCard(cbAllBomCardData, cbAllBomCardCount, cbTmpCard, cbHandCardCount) ;

			//三条扑克
			GetAllThreeCard(cbTurnCardData, cbTurnCardCount, cbTurnThreeCard, cbTurnThreeCount) ;

			unsigned char cbFirstCard = 0 ;

			//去除2和王
			if(cbTurnThreeCount>3)
				for(unsigned char i=0 ; i<cbHandCardCount-cbAllBomCardCount ; ++i)	
					if(GetCardLogicValue(cbTmpCard[i])<15)	
					{
						cbFirstCard = i ; 
						break ;
					}

					GetAllThreeCard(cbTmpCard+cbFirstCard, cbHandCardCount-cbFirstCard-cbAllBomCardCount, cbHandThreeCard, cbHandThreeCount) ;

					if(cbHandThreeCount<cbTurnThreeCount || (cbHandThreeCount>0&&GetCardLogicValue(cbHandThreeCard[0])<GetCardLogicValue(cbTurnThreeCard[0]))) return ;

					for(unsigned char i=0; i<cbHandThreeCount; i+=3)
					{
						unsigned char cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[i]);
						unsigned char cbThreeLineCard[MAX_COUNT];
						unsigned char cbThreeLineCardCount=3;
						cbThreeLineCard[0]=cbHandThreeCard[i];
						cbThreeLineCard[1]=cbHandThreeCard[i+1];
						cbThreeLineCard[2]=cbHandThreeCard[i+2];
						for(unsigned char j=i+3; j<cbHandThreeCount; j+=3)
						{
							//连续判断
							if(1!=(cbLastLogicCard-(GetCardLogicValue(cbHandThreeCard[j]))) || cbThreeLineCardCount==cbTurnThreeCount) break;

							cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[j]);
							cbThreeLineCard[cbThreeLineCardCount]=cbHandThreeCard[j];
							cbThreeLineCard[cbThreeLineCardCount+1]=cbHandThreeCard[j+1];
							cbThreeLineCard[cbThreeLineCardCount+2]=cbHandThreeCard[j+2];
							cbThreeLineCardCount += 3;
						}
						if(cbThreeLineCardCount==cbTurnThreeCount && GetCardLogicValue(cbThreeLineCard[0])>GetCardLogicValue(cbTurnThreeCard[0]))
						{
							unsigned char Index ;

							unsigned char cbRemainCard[MAX_COUNT];
							memcpy(cbRemainCard, cbTmpCard, (cbHandCardCount-cbAllBomCardCount)*sizeof(unsigned char));
							RemoveCard(cbThreeLineCard, cbTurnThreeCount, cbRemainCard, (cbHandCardCount-cbAllBomCardCount)) ;

							//单牌组合
							unsigned char cbComCard[5];
							unsigned char cbComResCard[254][5] ;
							unsigned char cbComResLen=0 ;
							Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, cbSingleCardCount, (cbHandCardCount-cbAllBomCardCount)-cbTurnThreeCount, cbSingleCardCount);
							for(unsigned char i=0; i<cbComResLen; ++i)
							{
								Index = CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount ;
								CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardType = CT_THREE_LINE_TAKE_ONE;
								//保存三条
								memcpy(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index], cbThreeLineCard, sizeof(unsigned char)*cbTurnThreeCount);
								//保存单牌
								memcpy(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index]+cbTurnThreeCount, cbComResCard[i], cbSingleCardCount) ;

								ASSERT(cbTurnThreeCount+cbSingleCardCount==cbTurnCardCount) ;
								CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbEachHandCardCount[Index] = cbTurnCardCount ;
								CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount++ ;

								ASSERT(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
							}

						}
					}

					break;
		}
	case CT_THREE_LINE_TAKE_TWO://三带一对
		{
			unsigned char cbTurnThreeCard[MAX_COUNT]; 
			unsigned char cbTurnThreeCount=0;
			unsigned char cbHandThreeCard[MAX_COUNT];
			unsigned char cbHandThreeCount=0 ;
			unsigned char cbDoubleCardCount=cbTurnCardCount/5;

			//三条扑克
			GetAllThreeCard(cbTurnCardData, cbTurnCardCount, cbTurnThreeCard, cbTurnThreeCount) ;

			unsigned char cbFirstCard = 0 ;

			//去除2和王
			if(cbTurnThreeCount>3)
				for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

				GetAllThreeCard(cbTmpCard+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandThreeCard, cbHandThreeCount) ;

				if(cbHandThreeCount<cbTurnThreeCount || (cbHandThreeCount>0&&GetCardLogicValue(cbHandThreeCard[0])<GetCardLogicValue(cbTurnThreeCard[0]))) return ;

				for(unsigned char i=0; i<cbHandThreeCount; i+=3)
				{
					unsigned char cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[i]);
					unsigned char cbThreeLineCard[MAX_COUNT];
					unsigned char cbThreeLineCardCount=3;
					cbThreeLineCard[0]=cbHandThreeCard[i];
					cbThreeLineCard[1]=cbHandThreeCard[i+1];
					cbThreeLineCard[2]=cbHandThreeCard[i+2];
					for(unsigned char j=i+3; j<cbHandThreeCount; j+=3)
					{
						//连续判断
						if(1!=(cbLastLogicCard-(GetCardLogicValue(cbHandThreeCard[j]))) || cbThreeLineCardCount==cbTurnThreeCount) break;

						cbLastLogicCard=GetCardLogicValue(cbHandThreeCard[j]);
						cbThreeLineCard[cbThreeLineCardCount]=cbHandThreeCard[j];
						cbThreeLineCard[cbThreeLineCardCount+1]=cbHandThreeCard[j+1];
						cbThreeLineCard[cbThreeLineCardCount+2]=cbHandThreeCard[j+2];
						cbThreeLineCardCount += 3;
					}
					if(cbThreeLineCardCount==cbTurnThreeCount && GetCardLogicValue(cbThreeLineCard[0])>GetCardLogicValue(cbTurnThreeCard[0]))
					{
						unsigned char Index ;

						unsigned char cbRemainCard[MAX_COUNT];
						memcpy(cbRemainCard, cbTmpCard, cbHandCardCount*sizeof(unsigned char));
						RemoveCard(cbThreeLineCard, cbTurnThreeCount, cbRemainCard, cbHandCardCount) ;

						unsigned char cbAllDoubleCardData[MAX_COUNT] ;
						unsigned char cbAllDoubleCardCount=0 ;
						GetAllDoubleCard(cbRemainCard, cbHandCardCount-cbTurnThreeCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;


						unsigned char cbDoubleCardIndex[10]; //对牌下标
						for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
							cbDoubleCardIndex[j]=i ;

						//对牌组合
						unsigned char cbComCard[5];
						unsigned char cbComResCard[254][5] ;
						unsigned char cbComResLen=0 ;

						//利用对牌的下标做组合，再根据下标提取出对牌
						Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, cbDoubleCardCount, cbAllDoubleCardCount/2, cbDoubleCardCount);
						for(unsigned char i=0; i<cbComResLen; ++i)
						{
							Index = CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount ;
							CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardType = CT_THREE_LINE_TAKE_TWO ;
							//保存三条
							memcpy(CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index], cbThreeLineCard, sizeof(unsigned char)*cbTurnThreeCount);
							//保存对牌
							for(unsigned char j=0, k=0; j<cbDoubleCardCount; ++j, k+=2)
							{
								CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][cbTurnThreeCount+k] = cbAllDoubleCardData[cbComResCard[i][j]];
								CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index][cbTurnThreeCount+k+1] = cbAllDoubleCardData[cbComResCard[i][j]+1];
							}

							ASSERT(cbTurnThreeCount+cbDoubleCardCount*2==cbTurnCardCount) ;
							CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbEachHandCardCount[Index] = cbTurnCardCount ;

							CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount++ ;

							ASSERT(CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount<=MAX_TYPE_COUNT) ;
						}		

					}
				}

				break;
		}
	case CT_FOUR_LINE_TAKE_ONE://四带两单
		{
			unsigned char cbFirstCard = 0 ;
			//去除王牌
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCard[i])!=0x40)	{cbFirstCard = i ; break ;}

			unsigned char cbHandAllFourCardData[MAX_COUNT] ;
			unsigned char cbHandAllFourCardCount=0;
			unsigned char cbTurnAllFourCardData[MAX_COUNT];
			unsigned char cbTurnAllFourCardCount=0;
			//抽取四张
			GetAllBomCard(cbTmpCard+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;
			GetAllBomCard(cbTurnCardData, cbTurnCardCount, cbTurnAllFourCardData, cbTurnAllFourCardCount) ;

			if(cbHandAllFourCardCount>0 && GetCardLogicValue(cbHandAllFourCardData[0])<GetCardLogicValue(cbTurnAllFourCardData[0])) return ;


			unsigned char cbCanOutFourCardData[MAX_COUNT] ;
			unsigned char cbCanOutFourCardCount=0 ;
			//可出的牌
			for(unsigned char i=0; i<cbHandAllFourCardCount; i+=4)
			{
				if(GetCardLogicValue(cbHandAllFourCardData[i])>GetCardLogicValue(cbTurnAllFourCardData[0]))
				{
					cbCanOutFourCardData[cbCanOutFourCardCount] = cbHandAllFourCardData[i] ;
					cbCanOutFourCardData[cbCanOutFourCardCount+1] = cbHandAllFourCardData[i+1] ;
					cbCanOutFourCardData[cbCanOutFourCardCount+2] = cbHandAllFourCardData[i+2] ;
					cbCanOutFourCardData[cbCanOutFourCardCount+3] = cbHandAllFourCardData[i+3] ;
					cbCanOutFourCardCount += 4 ;
				}
			}

			if((cbHandCardCount-cbCanOutFourCardCount) < (cbTurnCardCount-cbTurnAllFourCardCount)) return ;

			unsigned char cbRemainCard[MAX_COUNT];
			memcpy(cbRemainCard, cbTmpCard, cbHandCardCount*sizeof(unsigned char));
			RemoveCard(cbCanOutFourCardData, cbCanOutFourCardCount, cbRemainCard, cbHandCardCount) ;
			for(unsigned char Start=0; Start<cbCanOutFourCardCount; Start += 4)
			{
				unsigned char Index ;
				//单牌组合
				unsigned char cbComCard[5];
				unsigned char cbComResCard[254][5] ;
				unsigned char cbComResLen=0 ;
				//单牌组合
				Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, 2, cbHandCardCount-cbCanOutFourCardCount, 2);
				for(unsigned char i=0; i<cbComResLen; ++i)
				{
					//不能带对
					if(GetCardValue(cbComResCard[i][0])==GetCardValue(cbComResCard[i][1])) continue ;

					Index=CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardTypeCount ;
					CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardType = CT_FOUR_LINE_TAKE_ONE ;
					memcpy(CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardData[Index], cbCanOutFourCardData+Start, 4) ;
					CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardData[Index][4] = cbComResCard[i][0] ;
					CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardData[Index][4+1] = cbComResCard[i][1] ;
					CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbEachHandCardCount[Index] = 6 ;
					CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_FOUR_LINE_TAKE_ONE].cbCardTypeCount<=MAX_TYPE_COUNT) ;
				}
			}


			break;
		}
	case CT_FOUR_LINE_TAKE_TWO://四带两对
		{
			unsigned char cbFirstCard = 0 ;
			//去除王牌
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCard[i])!=0x40)	{cbFirstCard = i ; break ;}

			unsigned char cbHandAllFourCardData[MAX_COUNT] ;
			unsigned char cbHandAllFourCardCount=0;
			unsigned char cbTurnAllFourCardData[MAX_COUNT];
			unsigned char cbTurnAllFourCardCount=0;
			//抽取四张
			GetAllBomCard(cbTmpCard+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;
			GetAllBomCard(cbTurnCardData, cbTurnCardCount, cbTurnAllFourCardData, cbTurnAllFourCardCount) ;

			if(cbHandAllFourCardCount>0 && GetCardLogicValue(cbHandAllFourCardData[0])<GetCardLogicValue(cbTurnAllFourCardData[0])) return ;


			unsigned char cbCanOutFourCardData[MAX_COUNT] ;
			unsigned char cbCanOutFourCardCount=0 ;
			//可出的牌
			for(unsigned char i=0; i<cbHandAllFourCardCount; i+=4)
			{
				if(GetCardLogicValue(cbHandAllFourCardData[i])>GetCardLogicValue(cbTurnAllFourCardData[0]))
				{
					cbCanOutFourCardData[cbCanOutFourCardCount] = cbHandAllFourCardData[i] ;
					cbCanOutFourCardData[cbCanOutFourCardCount+1] = cbHandAllFourCardData[i+1] ;
					cbCanOutFourCardData[cbCanOutFourCardCount+2] = cbHandAllFourCardData[i+2] ;
					cbCanOutFourCardData[cbCanOutFourCardCount+3] = cbHandAllFourCardData[i+3] ;
					cbCanOutFourCardCount += 4 ;
				}
			}

			if((cbHandCardCount-cbCanOutFourCardCount) < (cbTurnCardCount-cbTurnAllFourCardCount)) return ;

			unsigned char cbRemainCard[MAX_COUNT];
			memcpy(cbRemainCard, cbTmpCard, cbHandCardCount*sizeof(unsigned char));
			RemoveCard(cbCanOutFourCardData, cbCanOutFourCardCount, cbRemainCard, cbHandCardCount) ;
			for(unsigned char Start=0; Start<cbCanOutFourCardCount; Start += 4)
			{
				unsigned char cbAllDoubleCardData[MAX_COUNT] ;
				unsigned char cbAllDoubleCardCount=0 ;
				GetAllDoubleCard(cbRemainCard, cbHandCardCount-cbCanOutFourCardCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;


				unsigned char cbDoubleCardIndex[10]; //对牌下标
				for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
					cbDoubleCardIndex[j]=i ;

				//对牌组合
				unsigned char cbComCard[5];
				unsigned char cbComResCard[254][5] ;
				unsigned char cbComResLen=0 ;

				//利用对牌的下标做组合，再根据下标提取出对牌
				Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, 2, cbAllDoubleCardCount/2, 2);
				for(unsigned char i=0; i<cbComResLen; ++i)
				{
					unsigned char Index = CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount ;
					CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardType = CT_FOUR_LINE_TAKE_TWO ;
					memcpy(CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index], cbCanOutFourCardData+Start, 4) ;

					//保存对牌
					for(unsigned char j=0, k=0; j<4; ++j, k+=2)
					{
						CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index][4+k] = cbAllDoubleCardData[cbComResCard[i][j]];
						CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index][4+k+1] = cbAllDoubleCardData[cbComResCard[i][j]+1];
					}

					CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbEachHandCardCount[Index] = 8 ;
					CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount<=MAX_TYPE_COUNT) ;
				}
			}
			break;
		}
	case CT_BOMB_CARD:			//炸弹类型
		{
			unsigned char cbAllBomCardData[MAX_COUNT] ;
			unsigned char cbAllBomCardCount=0 ; 
			GetAllBomCard(cbTmpCard, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount) ;
			unsigned char cbFirstBom=0 ;
			unsigned char Index ;
			if(cbAllBomCardCount>0 && cbAllBomCardData[0]==0x4F)
			{
				Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount ;
				CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD ;
				CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = 0x4F ;
				CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = 0x4E ;
				CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 2 ;
				CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++ ;

				ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount<=MAX_TYPE_COUNT) ;
				cbFirstBom=2;
			}
			for(unsigned char i=cbFirstBom; i<cbAllBomCardCount; i+=4)
			{
				if(GetCardLogicValue(cbAllBomCardData[i])>GetCardLogicValue(cbTurnCardData[0]))
				{
					Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount ;
					CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD ;
					memcpy(CardTypeResult[CT_BOMB_CARD].cbCardData[Index], cbAllBomCardData+i, 4) ;
					CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4 ;
					CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++ ;

					ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount<=MAX_TYPE_COUNT) ;
				}
			}
			break;
		}
	case CT_MISSILE_CARD:		//火箭类型
		{
			//没有比火箭更大的牌了
			break;
		}
	default:
		ASSERT(false) ;
		break;
	}

}

/********************************************************************
函数名：Combination
参数：
cbCombineCardData：存储单个的组合结果
cbResComLen：已得到的组合长度，开始调用时此参数为0
cbResultCardData：存储所有的组合结果
cbResCardLen：cbResultCardData的第一下标的长度，组合结果的个数
cbSrcCardData：需要做组合的数据
cbSrcLen：cbSrcCardData的数据数目
cbCombineLen2，cbCombineLen1：组合的长度，开始调用时两者相等。
*********************************************************************/
//组合算法
void CAndroidAI::Combination(unsigned char cbCombineCardData[], unsigned char cbResComLen,  unsigned char cbResultCardData[100][5], unsigned char &cbResCardLen,unsigned char cbSrcCardData[] , unsigned char cbCombineLen1, unsigned char cbSrcLen, const unsigned char cbCombineLen2)
{

	if( cbResComLen == cbCombineLen2 )
	{
		memcpy(&cbResultCardData[cbResCardLen], cbCombineCardData, cbResComLen) ;
		++cbResCardLen ;

		ASSERT(cbResCardLen<255) ;

	}
	else
	{ 
		if(cbCombineLen1 >= 1 && cbSrcLen > 0 && (cbSrcLen+1) >= 0 ){
			cbCombineCardData[cbCombineLen2-cbCombineLen1] =  cbSrcCardData[0];
			++cbResComLen ;
			Combination(cbCombineCardData,cbResComLen, cbResultCardData, cbResCardLen, cbSrcCardData+1,cbCombineLen1-1, cbSrcLen-1, cbCombineLen2);

			--cbResComLen;
			Combination(cbCombineCardData,cbResComLen, cbResultCardData, cbResCardLen, cbSrcCardData+1,cbCombineLen1, cbSrcLen-1, cbCombineLen2);
		}
	}
}

//排列算法
void CAndroidAI::Permutation(unsigned char *list, int m, int n, unsigned char result[][4], unsigned char &len)
{ 
	int j,temp; 
	if(m == n){ 
		for(j = 0; j < n; j++) 
			result[len][j]=list[j]; 
		len++ ;
	} 
	else{ 
		for(j = m; j < n; j++){ 
			temp = list[m] ;
			list[m] = list[j];
			list[j] = temp ;
			Permutation(list,m+1,n,result,len); 
			temp = list[m] ;
			list[m] = list[j];
			list[j] = temp ;
		} 
	} 
} 

//单牌个数
unsigned char CAndroidAI::AnalyseSinleCardCount(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char const cbWantOutCardData[], unsigned char const cbWantOutCardCount, unsigned char cbSingleCardData[])
{
	unsigned char cbRemainCard[MAX_COUNT] ;
	unsigned char cbRemainCardCount=0 ;
	memcpy(cbRemainCard, cbHandCardData, cbHandCardCount) ;
	SortCardList(cbRemainCard, cbHandCardCount, ST_ORDER) ;

	if(cbWantOutCardCount!=0) RemoveCard(cbWantOutCardData, cbWantOutCardCount, cbRemainCard, cbHandCardCount) ;
	cbRemainCardCount = cbHandCardCount-cbWantOutCardCount ;

	//函数指针
	typedef void (CAndroidAI::*pGetAllCardFun)(unsigned char const [], unsigned char const , unsigned char[], unsigned char &); 

	//指针数组
	pGetAllCardFun GetAllCardFunArray[4] ;
	GetAllCardFunArray[0] = &CAndroidAI::GetAllBomCard;		//炸弹函数
	GetAllCardFunArray[1] = &CAndroidAI::GetAllLineCard;	//顺子函数
	GetAllCardFunArray[2] = &CAndroidAI::GetAllThreeCard;	//三条函数
	GetAllCardFunArray[3] = &CAndroidAI::GetAllDoubleCard;	//对子函数

	//指针数组下标
	unsigned char cbIndexArray[4] = {0,1,2,3} ;
	//排列结果
	unsigned char cbPermutationRes[24][4] ;
	unsigned char cbLen=0 ;
	//计算排列
	Permutation(cbIndexArray, 0, 4, cbPermutationRes, cbLen) ;
	ASSERT(cbLen==24) ;
	if(cbLen!=24) return MAX_COUNT ;

	//单牌数目
	unsigned char cbMinSingleCardCount = MAX_COUNT ;
	//计算最小值
	for(unsigned char i=0; i<24; ++i)
	{
		//保留数据
		unsigned char cbTmpCardData[MAX_COUNT] ;
		unsigned char cbTmpCardCount = cbRemainCardCount ;
		memcpy(cbTmpCardData, cbRemainCard, cbRemainCardCount) ;

		for(unsigned char j=0; j<4; ++j)
		{
			unsigned char Index = cbPermutationRes[i][j] ;

			//校验下标
			ASSERT(Index>=0 && Index<4) ;
			if(Index<0 || Index>=4) return MAX_COUNT ;

			pGetAllCardFun pTmpGetAllCardFun = GetAllCardFunArray[Index] ;

			//提取扑克
			unsigned char cbGetCardData[MAX_COUNT] ;
			unsigned char cbGetCardCount=0 ;
			//成员函数
			((*this).*pTmpGetAllCardFun)(cbTmpCardData, cbTmpCardCount, cbGetCardData, cbGetCardCount) ;

			//删除扑克
			if(cbGetCardCount!=0) RemoveCard(cbGetCardData, cbGetCardCount, cbTmpCardData, cbTmpCardCount) ;
			cbTmpCardCount -= cbGetCardCount ;
		}

		//计算单牌
		unsigned char cbSingleCard[MAX_COUNT] ;
		unsigned char cbSingleCardCount=0 ;
		GetAllSingleCard(cbTmpCardData, cbTmpCardCount, cbSingleCard, cbSingleCardCount) ;
		cbMinSingleCardCount = cbMinSingleCardCount > cbSingleCardCount ? cbSingleCardCount : cbMinSingleCardCount ;
	}

	return cbMinSingleCardCount ;
}


//地主出牌（先出牌）
void CAndroidAI::BankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, tagOutCardResult & OutCardResult) 
{
	//零下标没用
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//初始变量
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	unsigned char cbLineCard[MAX_COUNT] ;
	unsigned char cbThreeLineCard[MAX_COUNT] ;
	unsigned char cbDoubleLineCard[MAX_COUNT] ;
	unsigned char cbLineCardCount;
	unsigned char cbThreeLineCardCount ;
	unsigned char cbDoubleLineCount ;
	GetAllLineCard(cbHandCardData, cbHandCardCount, cbLineCard, cbLineCardCount) ;
	GetAllThreeCard(cbHandCardData, cbHandCardCount, cbThreeLineCard, cbThreeLineCardCount) ;
	GetAllDoubleCard(cbHandCardData, cbHandCardCount, cbDoubleLineCard, cbDoubleLineCount) ;

	int wUndersideOfBanker = (m_wBankerUser+1)%GAME_PLAYER ;	//地主下家
	int wUpsideOfBanker = (wUndersideOfBanker+1)%GAME_PLAYER ;	//地主上家

	//如果只剩顺牌和单只，则先出顺
	{
		if(cbLineCardCount+1==cbHandCardCount && CT_SINGLE==GetCardType(cbLineCard, cbLineCardCount))
		{
			OutCardResult.cbCardCount = cbLineCardCount ;
			memcpy(OutCardResult.cbResultCard, cbLineCard, cbLineCardCount) ;
		}
		else if(cbThreeLineCardCount+1==cbHandCardCount && CT_THREE_LINE==GetCardType(cbThreeLineCard, cbThreeLineCardCount))
		{
			OutCardResult.cbCardCount = cbThreeLineCardCount ;
			memcpy(OutCardResult.cbResultCard, cbThreeLineCard, cbThreeLineCardCount) ;
		}
		else if(cbDoubleLineCount+1==cbHandCardCount && CT_DOUBLE_LINE==GetCardType(cbDoubleLineCard, cbDoubleLineCount))
		{
			OutCardResult.cbCardCount = cbDoubleLineCount ;
			memcpy(OutCardResult.cbResultCard, cbDoubleLineCard, cbDoubleLineCount) ;
		}
		//双王炸弹和一手
		else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
		{
			OutCardResult.cbCardCount = 2 ;
			OutCardResult.cbResultCard[0] = 0x4f ;
			OutCardResult.cbResultCard[1] = 0x4e ;
		}

		if(OutCardResult.cbCardCount>0)
		{
			return ;
		}
	}

	//对王加一只
	if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//对王
	else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//只剩一手牌
	else if(CT_ERROR!=GetCardType(cbHandCardData, cbHandCardCount))
	{
		OutCardResult.cbCardCount = cbHandCardCount ;
		memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount) ;
		return ;
	}

	//只剩一张和一手
	if(cbHandCardCount>=2)
	{
		//上家扑克
		tagOutCardTypeResult UpsideCanOutCardType1[13] ;
		my_memset(UpsideCanOutCardType1, sizeof(UpsideCanOutCardType1)) ;
		tagOutCardTypeResult UpsideCanOutCardType2[13] ;
		my_memset(UpsideCanOutCardType2, sizeof(UpsideCanOutCardType2)) ;

		//下家扑克
		tagOutCardTypeResult UndersideCanOutCardType1[13] ;
		my_memset(UndersideCanOutCardType1, sizeof(UndersideCanOutCardType1)) ;
		tagOutCardTypeResult UndersideCanOutCardType2[13] ;
		my_memset(UndersideCanOutCardType2, sizeof(UndersideCanOutCardType2)) ;

		unsigned char cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount-1) ;
		unsigned char cbSecondHandCardType = GetCardType(cbHandCardData+1, cbHandCardCount-1) ;

		if(CT_ERROR!=cbFirstHandCardType && cbFirstHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbFirstHandCardType!= CT_FOUR_LINE_TAKE_TWO)
		{
			AnalyseOutCardType(m_cbAllCardData[wUpsideOfBanker], m_cbUserCardCount[wUpsideOfBanker], cbHandCardData, cbHandCardCount-1, UpsideCanOutCardType1) ;
			AnalyseOutCardType(m_cbAllCardData[wUndersideOfBanker], m_cbUserCardCount[wUndersideOfBanker], cbHandCardData, cbHandCardCount-1, UndersideCanOutCardType1) ;
		}
		if(CT_ERROR!=cbSecondHandCardType && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO)
		{
			AnalyseOutCardType(m_cbAllCardData[wUpsideOfBanker], m_cbUserCardCount[wUpsideOfBanker], cbHandCardData+1, cbHandCardCount-1, UpsideCanOutCardType2) ;
			AnalyseOutCardType(m_cbAllCardData[wUndersideOfBanker], m_cbUserCardCount[wUndersideOfBanker], cbHandCardData+1, cbHandCardCount-1, UndersideCanOutCardType2) ;
		}

		if(cbSecondHandCardType!=CT_ERROR && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO && UpsideCanOutCardType2[cbSecondHandCardType].cbCardTypeCount==0 && UndersideCanOutCardType2[cbSecondHandCardType].cbCardTypeCount==0 && 
			UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0 && UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData+1, cbHandCardCount-1) ;
			return ;
		}

		if(cbFirstHandCardType!=CT_ERROR && cbFirstHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbFirstHandCardType!= CT_FOUR_LINE_TAKE_TWO && UpsideCanOutCardType1[cbFirstHandCardType].cbCardTypeCount==0 && UndersideCanOutCardType1[cbFirstHandCardType].cbCardTypeCount==0 &&
			UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0 && UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount-1) ;
			return ;
		}

		if(GetCardLogicValue(cbHandCardData[0])>=GetCardLogicValue(m_cbAllCardData[wUpsideOfBanker][0]) &&
			GetCardLogicValue(cbHandCardData[0])>=GetCardLogicValue(m_cbAllCardData[wUndersideOfBanker][0]) &&
			CT_ERROR!=cbSecondHandCardType && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0 && UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = 1 ;
			OutCardResult.cbResultCard[0] = cbHandCardData[0] ;
			return ;
		}

		if(CT_ERROR!=cbSecondHandCardType && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0 && UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData+1, cbHandCardCount-1) ;
			return ;
		}
	}



	{
		{
			//分析扑克
			tagOutCardTypeResult MeOutCardTypeResult[13] ;
			my_memset(MeOutCardTypeResult, sizeof(MeOutCardTypeResult)) ;
			AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;

			//计算单牌
			unsigned char cbMinSingleCardCount[4] ;
			cbMinSingleCardCount[0]=MAX_COUNT ;
			cbMinSingleCardCount[1]=MAX_COUNT ;
			cbMinSingleCardCount[2]=MAX_COUNT ;
			cbMinSingleCardCount[3]=MAX_COUNT ;
			unsigned char cbIndex[4]={0} ;
			unsigned char cbOutcardType[4]={CT_ERROR} ;
			unsigned char cbMinValue=MAX_COUNT ; 
			unsigned char cbMinSingleCountInFour=MAX_COUNT ;
			unsigned char cbMinCardType=CT_ERROR ;
			unsigned char cbMinIndex=0 ;

			//除炸弹外的牌
			for(unsigned char cbCardType=CT_DOUBLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
			{

				tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType] ;

				//相同牌型，相同长度，单连，对连等相同牌型可能长度不一样
				unsigned char cbThisHandCardCount = MAX_COUNT ;

				//上家扑克
				tagOutCardTypeResult UpsideOutCardTypeResult[13] ;
				my_memset(UpsideOutCardTypeResult, sizeof(UpsideOutCardTypeResult)) ;

				//下家扑克
				tagOutCardTypeResult UndersideOutCardTypeResult[13] ;
				my_memset(UndersideOutCardTypeResult, sizeof(UndersideOutCardTypeResult)) ;


				for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
				{
					unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

					//重新分析
					if(tmpCardResult.cbEachHandCardCount[i]!=cbThisHandCardCount)
					{
						cbThisHandCardCount = tmpCardResult.cbEachHandCardCount[i] ;
						AnalyseOutCardType(m_cbAllCardData[wUpsideOfBanker], m_cbUserCardCount[wUpsideOfBanker], 
							tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,UpsideOutCardTypeResult) ;
						AnalyseOutCardType(m_cbAllCardData[wUndersideOfBanker], m_cbUserCardCount[wUndersideOfBanker], 
							tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,UndersideOutCardTypeResult) ;
					}
					unsigned char cbMaxValue=0 ; 
					unsigned char Index = 0 ;

					//敌方可以压住牌
					if(UpsideOutCardTypeResult[cbCardType].cbCardTypeCount>0 || UndersideOutCardTypeResult[cbCardType].cbCardTypeCount>0)
					{
						continue ;
					}
					//是否有大牌 
					if(tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
					{
						bool bHaveLargeCard=false ;
						for(unsigned char j=0; j<tmpCardResult.cbEachHandCardCount[i]; ++j)
						{
							if(GetCardLogicValue(tmpCardResult.cbCardData[i][j])>=15) bHaveLargeCard=true ;
							if(cbCardType!=CT_SINGLE_LINE && cbCardType!=CT_DOUBLE_LINE  && GetCardLogicValue(tmpCardResult.cbCardData[i][0])==14) bHaveLargeCard=true ; 
						}

						if(bHaveLargeCard)
							continue ;
					}

					//搜索cbMinSingleCardCount[4]的最大值
					for(unsigned char j=0; j<4; ++j)
					{
						if(cbMinSingleCardCount[j]>=cbTmpCount)
						{
							cbMinSingleCardCount[j] = cbTmpCount ;
							cbIndex[j] = i ;
							cbOutcardType[j] = cbCardType ;
							break ;
						}
					}

					//保存最小值
					if(cbMinSingleCountInFour>=cbTmpCount)
					{
						//最小牌型
						cbMinCardType = cbCardType ;
						//最小牌型中的最小单牌
						cbMinSingleCountInFour=cbTmpCount ;						
						//最小牌型中的最小牌
						cbMinIndex=i ;
					}
				}
			}

			if(cbMinSingleCountInFour>=AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0)+3 && 
				(m_cbUserCardCount[wUndersideOfBanker]>=4 && m_cbUserCardCount[wUpsideOfBanker]>=4))
				cbMinSingleCountInFour=MAX_COUNT ;

			if(cbMinSingleCountInFour!=MAX_COUNT)
			{
				unsigned char Index = cbMinIndex ;

				//选择最小牌
				for(unsigned char i=0; i<4; ++i)
				{
					if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
						GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
						Index = cbIndex[i] ;
				}

				//对王加一只
				if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
				{
					OutCardResult.cbCardCount = 2 ;
					OutCardResult.cbResultCard[0] = 0x4f ;
					OutCardResult.cbResultCard[1] = 0x4e ;
					return ;
				}
				//对王
				else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
				{
					OutCardResult.cbCardCount = 2 ;
					OutCardResult.cbResultCard[0] = 0x4f ;
					OutCardResult.cbResultCard[1] = 0x4e ;
					return ;
				}
				else
				{
					//设置变量
					OutCardResult.cbCardCount=MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
					memcpy(OutCardResult.cbResultCard,MeOutCardTypeResult[cbMinCardType].cbCardData[Index],MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
					return ;
				}

				ASSERT(OutCardResult.cbCardCount>0) ;

				return ;
			}

			//如果地主扑克少于5，还没有找到适合的牌则从大出到小
			if(OutCardResult.cbCardCount<=0 && (m_cbUserCardCount[wUndersideOfBanker]>=4 || m_cbUserCardCount[wUpsideOfBanker]>=4))
			{
				//只有一张牌时不能放地主走
				if(m_cbUserCardCount[m_wBankerUser]==1 && MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount>0)
				{
					unsigned char Index=MAX_COUNT ;
					for(unsigned char i=0; i<MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount; ++i)
					{
						if(GetCardLogicValue(MeOutCardTypeResult[CT_SINGLE].cbCardData[i][0])>=GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]))
						{
							Index=i ;
						}
						else break ;
					}

					if(MAX_COUNT!=Index)
					{
						OutCardResult.cbCardCount = MeOutCardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] ;
						memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[CT_SINGLE].cbCardData[Index], OutCardResult.cbCardCount) ;
						return ;
					}
				}
			}
		}
	}
	unsigned char cbFirstCard=0 ;
	//过滤王和2
	for(unsigned char i=0; i<cbHandCardCount; ++i) 
		if(GetCardLogicValue(cbHandCardData[i])<15)
		{
			cbFirstCard = i ;
			break ;
		}

		if(cbFirstCard<cbHandCardCount-1)
			AnalyseOutCardType(cbHandCardData+cbFirstCard, cbHandCardCount-cbFirstCard, CardTypeResult) ;
		else
			AnalyseOutCardType(cbHandCardData, cbHandCardCount, CardTypeResult) ;

		//计算单牌
		unsigned char cbMinSingleCardCount[4] ;
		cbMinSingleCardCount[0]=MAX_COUNT ;
		cbMinSingleCardCount[1]=MAX_COUNT ;
		cbMinSingleCardCount[2]=MAX_COUNT ;
		cbMinSingleCardCount[3]=MAX_COUNT ;
		unsigned char cbIndex[4]={0} ;
		unsigned char cbOutcardType[4]={CT_ERROR} ;
		unsigned char cbMinValue=MAX_COUNT ; 
		unsigned char cbMinSingleCountInFour=MAX_COUNT ;
		unsigned char cbMinCardType=CT_ERROR ;
		unsigned char cbMinIndex=0 ;

		//除炸弹外的牌
		for(unsigned char cbCardType=CT_SINGLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType] ;
			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//搜索cbMinSingleCardCount[4]的最大值
				for(unsigned char j=0; j<4; ++j)
				{
					if(cbMinSingleCardCount[j]>=cbTmpCount)
					{
						cbMinSingleCardCount[j] = cbTmpCount ;
						cbIndex[j] = i ;
						cbOutcardType[j] = cbCardType ;
						break ;
					}
				}

				//保存最小值
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//最小牌型
					cbMinCardType = cbCardType ;
					//最小牌型中的最小单牌
					cbMinSingleCountInFour=cbTmpCount ;						
					//最小牌型中的最小牌
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//选择最小牌
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//对王加一只
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//对王
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//设置变量
				OutCardResult.cbCardCount=CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[cbMinCardType].cbCardData[Index],CardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}
		//如果只剩炸弹
		else
		{
			unsigned char Index=0 ;
			unsigned char cbLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			//最小炸弹
			for(unsigned char i=0; i<CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
				if(cbLogicCardValue>GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
				{
					cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]) ;
					Index = i ;
				}

				//设置变量
				OutCardResult.cbCardCount=CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[CT_BOMB_CARD].cbCardData[Index],CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index]*sizeof(unsigned char));

				return ;
		}

		//如果都没有搜索到就出最小的一张
		OutCardResult.cbCardCount = 1 ;
		OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount-1] ;

		return ;
}

//地主出牌（后出牌）
void CAndroidAI::BankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult) 
{
	//初始变量
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//零下标没用
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//出牌类型
	unsigned char cbOutCardType = GetCardType(cbTurnCardData,cbTurnCardCount) ;
	AnalyseOutCardType(cbHandCardData,cbHandCardCount,cbTurnCardData,cbTurnCardCount, CardTypeResult) ;

	int wUndersideUser = (m_wBankerUser+1)%GAME_PLAYER ;
	int wUpsideUser = (wUndersideUser+1)%GAME_PLAYER ;

	//只剩炸弹
	if(cbHandCardCount==CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
	{
		OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0] ;
		memcpy(OutCardResult.cbResultCard,  CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount) ;

		return ;
	}
	//双王炸弹和一手
	else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return  ;
	}

	//取出四个最小单牌
	unsigned char cbMinSingleCardCount[4] ;
	cbMinSingleCardCount[0]=MAX_COUNT ;
	cbMinSingleCardCount[1]=MAX_COUNT ;
	cbMinSingleCardCount[2]=MAX_COUNT ;
	cbMinSingleCardCount[3]=MAX_COUNT ;
	unsigned char cbIndex[4]={0} ;	
	unsigned char cbMinSingleCountInFour=MAX_COUNT ;

	//可出扑克（这里已经过滤掉炸弹了）
	tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType] ;

	for(unsigned char i=0; i<CanOutCard.cbCardTypeCount; ++i)
	{
		//最小单牌
		unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,CanOutCard.cbCardData[i], CanOutCard.cbEachHandCardCount[i]) ; 
		unsigned char cbMaxValue=0 ; 
		unsigned char Index = 0 ;

		//搜索cbMinSingleCardCount[4]的最大值
		for(unsigned char j=0; j<4; ++j)
		{
			if(cbMinSingleCardCount[j]>=cbTmpCount)
			{
				cbMinSingleCardCount[j] = cbTmpCount ;
				cbIndex[j] = i ;
				break ;
			}
		}

	}

	for(unsigned char i=0; i<4; ++i)
		if(cbMinSingleCountInFour>cbMinSingleCardCount[i]) cbMinSingleCountInFour = cbMinSingleCardCount[i] ;


	//原始单牌数
	unsigned char cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,NULL,0) ;

	if(CanOutCard.cbCardTypeCount>0)
	{
		unsigned char cbMinLogicCardValue = GetCardLogicValue(0x4F)+1 ;
		bool bFindCard = false ;
		unsigned char cbCanOutIndex=0 ;
		for(unsigned char i=0; i<4; ++i)
		{
			unsigned char Index = cbIndex[i] ;

			if((cbMinSingleCardCount[i]<cbOriginSingleCardCount+3)  && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
				cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
			{
				//针对大牌
				bool bNoLargeCard = true ;

				//当出牌玩家手上牌数大于4，而且出的是小于K的牌而且不是出牌手上的最大牌时，不能出2去打
				if(m_cbUserCardCount[wOutCardUser]>=4 && cbHandCardCount>=5  && CanOutCard.cbEachHandCardCount[Index]>=2 && 
					GetCardLogicValue(CanOutCard.cbCardData[Index][0])>=15 &&
					GetCardLogicValue(cbTurnCardData[0])<13 &&
					(wOutCardUser==wUndersideUser&&GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]) || wOutCardUser==wUpsideUser&&GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[wUpsideUser][0])) && 
					CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
					bNoLargeCard=false ;

				//搜索有没有大牌（针对飞机带翅膀后面的带牌）
				for(unsigned char k=3; k<CanOutCard.cbEachHandCardCount[Index]; ++k)
				{
					if(GetCardLogicValue(CanOutCard.cbCardData[Index][k])>=15 && 
						CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
						bNoLargeCard = false ;
				}
				if(bNoLargeCard)
				{
					bFindCard = true ;
					cbCanOutIndex = Index ; 
					cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]) ;
				}
			}
		}

		if(bFindCard)
		{
			//最大牌
			unsigned char cbLargestLogicCard ;
			if(wOutCardUser==wUndersideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]) ;
			else if(wOutCardUser==wUpsideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUpsideUser][0]) ;
			bool bCanOut=true ;

			//王只压2
			if(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard)
			{
				if(GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0])==0x40 && GetCardLogicValue(cbTurnCardData[0])<=14 && cbHandCardCount>5) 								
				{
					bCanOut = false ;
				}
			}

			if(bCanOut)
			{
				//设置变量
				OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
				memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

				return ;
			}
		}

		if(cbOutCardType==CT_SINGLE)
		{
			//闲家的最大牌
			unsigned char cbLargestLogicCard ;
			if(wOutCardUser==wUndersideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]) ;
			else if(wOutCardUser==wUpsideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUpsideUser][0]) ;

			if(GetCardLogicValue(cbTurnCardData[0])==14 || 
				GetCardLogicValue(cbTurnCardData[0])>=cbLargestLogicCard || 
				(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard-1) ||
				(wOutCardUser==wUndersideUser&&m_cbUserCardCount[wUndersideUser]<=5 || wOutCardUser==wUpsideUser&&m_cbUserCardCount[wUpsideUser]<=5))
			{
				//取一张大于等于2而且要比闲家出的牌大的牌，
				unsigned char cbIndex=MAX_COUNT ;
				for(unsigned char i=0; i<cbHandCardCount; ++i)
					if(GetCardLogicValue(cbHandCardData[i])>GetCardLogicValue(cbTurnCardData[0]) &&
						GetCardLogicValue(cbHandCardData[i])>=15)
					{
						cbIndex = i ;
					}
					if(cbIndex!=MAX_COUNT)
					{
						//设置变量
						OutCardResult.cbCardCount=1;
						OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex] ;

						return ;
					}
			}
		}


		unsigned char cbMinSingleCount=MAX_COUNT ;
		unsigned char Index=0 ;
		for(unsigned char i=0; i<CardTypeResult[cbOutCardType].cbCardTypeCount; ++i)
		{
			unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, CardTypeResult[cbOutCardType].cbCardData[i], CardTypeResult[cbOutCardType].cbEachHandCardCount[i]) ;
			if(cbMinSingleCount>=cbTmpCount)
			{
				cbMinSingleCount = cbTmpCount ;
				Index = i ;
			}
		}
		//设置变量
		OutCardResult.cbCardCount=CardTypeResult[cbOutCardType].cbEachHandCardCount[Index];
		memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index], OutCardResult.cbCardCount) ;

		return ;
	}

	//还要考虑炸弹
	if(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount>0 && cbHandCardCount<=10)
	{
		tagOutCardTypeResult const &BomCard = CardTypeResult[CT_BOMB_CARD] ;
		unsigned char cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[0][0]) ;
		unsigned char Index = 0 ;
		for(unsigned char i=0; i<BomCard.cbCardTypeCount; ++i)
		{
			if(cbMinLogicValue>GetCardLogicValue(BomCard.cbCardData[i][0]))
			{
				cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[i][0]) ;
				Index = i ;
			}
		}

		//判断出了炸弹后的单牌数
		unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]) ;
		if(cbSingleCardCount>=3 || (cbOutCardType==CT_SINGLE && GetCardLogicValue(cbTurnCardData[0])<15)) return ;

		//设置变量
		OutCardResult.cbCardCount=BomCard.cbEachHandCardCount[Index];
		memcpy(OutCardResult.cbResultCard,BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]*sizeof(unsigned char));

		return ;
	}

	return ;
}

//地主上家（先出牌）
void CAndroidAI::UpsideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wMeChairID, tagOutCardResult & OutCardResult)
{
	//零下标没用
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//初始变量
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	unsigned char cbLineCard[MAX_COUNT] ;
	unsigned char cbThreeLineCard[MAX_COUNT] ;
	unsigned char cbDoubleLineCard[MAX_COUNT] ;
	unsigned char cbLineCardCount;
	unsigned char cbThreeLineCardCount ;
	unsigned char cbDoubleLineCount ;
	GetAllLineCard(cbHandCardData, cbHandCardCount, cbLineCard, cbLineCardCount) ;
	GetAllThreeCard(cbHandCardData, cbHandCardCount, cbThreeLineCard, cbThreeLineCardCount) ;
	GetAllDoubleCard(cbHandCardData, cbHandCardCount, cbDoubleLineCard, cbDoubleLineCount) ;

	//如果有顺牌和单只或一对，而且单只或对比地主的小，则先出顺
	{
		if(cbLineCardCount+1==cbHandCardCount && CT_SINGLE==GetCardType(cbLineCard, cbLineCardCount))
		{
			OutCardResult.cbCardCount = cbLineCardCount ;
			memcpy(OutCardResult.cbResultCard, cbLineCard, cbLineCardCount) ;
		}
		else if(cbThreeLineCardCount+1==cbHandCardCount && CT_THREE_LINE==GetCardType(cbThreeLineCard, cbThreeLineCardCount))
		{
			OutCardResult.cbCardCount = cbThreeLineCardCount ;
			memcpy(OutCardResult.cbResultCard, cbThreeLineCard, cbThreeLineCardCount) ;
		}
		else if(cbDoubleLineCount+1==cbHandCardCount && CT_DOUBLE_LINE==GetCardType(cbDoubleLineCard, cbDoubleLineCount))
		{
			OutCardResult.cbCardCount = cbDoubleLineCount ;
			memcpy(OutCardResult.cbResultCard, cbDoubleLineCard, cbDoubleLineCount) ;
		}
		//双王炸弹和一手
		else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
		{
			OutCardResult.cbCardCount = 2 ;
			OutCardResult.cbResultCard[0] = 0x4f ;
			OutCardResult.cbResultCard[1] = 0x4e ;
		}

		if(OutCardResult.cbCardCount>0)
		{
			return ;
		}
	}
	//对王加一只
	if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//对王
	else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//只剩一手牌
	else if(CT_ERROR!=GetCardType(cbHandCardData, cbHandCardCount))
	{
		OutCardResult.cbCardCount = cbHandCardCount ;
		memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount) ;
		return ;
	}

	//只剩一张和一手
	if(cbHandCardCount>=2)
	{
		//地主扑克
		tagOutCardTypeResult BankerCanOutCardType1[13] ;
		my_memset(BankerCanOutCardType1, sizeof(BankerCanOutCardType1)) ;
		tagOutCardTypeResult BankerCanOutCardType2[13] ;
		my_memset(BankerCanOutCardType2, sizeof(BankerCanOutCardType2)) ;

		unsigned char cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount-1) ;
		unsigned char cbSecondHandCardType = GetCardType(cbHandCardData+1, cbHandCardCount-1) ;

		//地主可以出的牌
		if(cbFirstHandCardType!=CT_ERROR)
			AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], cbHandCardData, cbHandCardCount-1, BankerCanOutCardType1) ;
		if(cbSecondHandCardType!=CT_ERROR)
			AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], cbHandCardData+1, cbHandCardCount-1, BankerCanOutCardType2) ;

		if(cbSecondHandCardType!=CT_ERROR && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO && 
			BankerCanOutCardType2[cbSecondHandCardType].cbCardTypeCount==0 && BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData+1, cbHandCardCount-1) ;
			return ;
		}

		if(cbFirstHandCardType!=CT_ERROR && cbFirstHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbFirstHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			BankerCanOutCardType1[cbFirstHandCardType].cbCardTypeCount==0 && BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount-1) ;
			return ;
		}

		if(GetCardLogicValue(cbHandCardData[0])>=GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) &&
			CT_ERROR!=cbSecondHandCardType && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = 1 ;
			OutCardResult.cbResultCard[0] = cbHandCardData[0] ;
			return ;
		}

		if(CT_ERROR!=cbSecondHandCardType && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData+1, cbHandCardCount-1) ;
			return ;
		}
	}


	//下家为地主，而且地主扑克少于5张
	//	if(m_cbUserCardCount[m_wBankerUser]<=5)
	{
		//分析扑克
		tagOutCardTypeResult MeOutCardTypeResult[13] ;
		my_memset(MeOutCardTypeResult, sizeof(MeOutCardTypeResult)) ;
		AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;

		//对家扑克
		int wFriendID ;
		for(int wChairID=0; wChairID<GAME_PLAYER; ++wChairID) 
			if(wChairID!=m_wBankerUser && wMeChairID!=wChairID) wFriendID = wChairID ;

		//计算单牌
		unsigned char cbMinSingleCardCount[4] ;
		cbMinSingleCardCount[0]=MAX_COUNT ;
		cbMinSingleCardCount[1]=MAX_COUNT ;
		cbMinSingleCardCount[2]=MAX_COUNT ;
		cbMinSingleCardCount[3]=MAX_COUNT ;
		unsigned char cbIndex[4]={0} ;
		unsigned char cbOutcardType[4]={CT_ERROR} ;
		unsigned char cbMinValue=MAX_COUNT ; 
		unsigned char cbMinSingleCountInFour=MAX_COUNT ;
		unsigned char cbMinCardType=CT_ERROR ;
		unsigned char cbMinIndex=0 ;

		//除炸弹外的牌
		for(unsigned char cbCardType=CT_DOUBLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType] ;

			//相同牌型，相同长度，单连，对连等相同牌型可能长度不一样
			unsigned char cbThisHandCardCount = MAX_COUNT ;

			//地主扑克
			tagOutCardTypeResult BankerCanOutCard[13] ;
			my_memset(BankerCanOutCard, sizeof(BankerCanOutCard)) ;

			tagOutCardTypeResult FriendOutCardTypeResult[13] ;
			my_memset(FriendOutCardTypeResult, sizeof(FriendOutCardTypeResult)) ;

			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				//重新分析
				if(tmpCardResult.cbEachHandCardCount[i]!=cbThisHandCardCount)
				{
					AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], 
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,BankerCanOutCard) ;
					AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID],
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,FriendOutCardTypeResult) ;
				}

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//地主可以压牌，而且队友不可以压地主
				if((BankerCanOutCard[cbCardType].cbCardTypeCount>0&&FriendOutCardTypeResult[cbCardType].cbCardTypeCount==0) || (BankerCanOutCard[cbCardType].cbCardTypeCount>0 && FriendOutCardTypeResult[cbCardType].cbCardTypeCount>0 &&
					GetCardLogicValue(FriendOutCardTypeResult[cbCardType].cbCardData[0][0])<=GetCardLogicValue(BankerCanOutCard[cbCardType].cbCardData[0][0])))
				{
					continue ;
				}
				//是否有大牌
				if(tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
				{
					bool bHaveLargeCard=false ;
					for(unsigned char j=0; j<tmpCardResult.cbEachHandCardCount[i]; ++j)
						if(GetCardLogicValue(tmpCardResult.cbCardData[i][j])>=15) bHaveLargeCard=true ;
					if(cbCardType!=CT_SINGLE_LINE && cbCardType!=CT_DOUBLE_LINE  && GetCardLogicValue(tmpCardResult.cbCardData[i][0])==14) bHaveLargeCard=true ; 

					if(bHaveLargeCard) continue ;
				}

				//地主是否可以走掉，这里都没有考虑炸弹
				if(tmpCardResult.cbEachHandCardCount[i]==m_cbUserCardCount[m_wBankerUser] &&
					GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0])>GetCardLogicValue(tmpCardResult.cbCardData[i][0])) continue ;

				//搜索cbMinSingleCardCount[4]的最大值
				for(unsigned char j=0; j<4; ++j)
				{
					if(cbMinSingleCardCount[j]>=cbTmpCount)
					{
						cbMinSingleCardCount[j] = cbTmpCount ;
						cbIndex[j] = i ;
						cbOutcardType[j] = cbCardType ;
						break ;
					}
				}

				//保存最小值
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//最小牌型
					cbMinCardType = cbCardType ;
					//最小牌型中的最小单牌
					cbMinSingleCountInFour=cbTmpCount ;						
					//最小牌型中的最小牌
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour>=AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0)+3 && 
			m_cbUserCardCount[m_wBankerUser]>4)
			cbMinSingleCountInFour=MAX_COUNT ;

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//选择最小牌
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//对王加一只
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//对王
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//设置变量
				OutCardResult.cbCardCount=MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,MeOutCardTypeResult[cbMinCardType].cbCardData[Index],MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}

		//如果地主扑克少于5，还没有找到适合的牌则从大出到小
		if(OutCardResult.cbCardCount<=0 && m_cbUserCardCount[m_wBankerUser]<=5)
		{
			//只有一张牌时不能放地主走
			if(m_cbUserCardCount[m_wBankerUser]==1 && MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount>0)
			{
				unsigned char Index=MAX_COUNT ;
				for(unsigned char i=0; i<MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount; ++i)
				{
					if(GetCardLogicValue(MeOutCardTypeResult[CT_SINGLE].cbCardData[i][0])>=GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]))
					{
						Index=i ;
					}
					else break ;
				}

				if(MAX_COUNT!=Index)
				{
					OutCardResult.cbCardCount = MeOutCardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] ;
					memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[CT_SINGLE].cbCardData[Index], OutCardResult.cbCardCount) ;
					return ;
				}
			}
		}
	}

	unsigned char cbFirstCard=0 ;
	//过滤王和2
	for(unsigned char i=0; i<cbHandCardCount; ++i) 
		if(GetCardLogicValue(cbHandCardData[i])<15)
		{
			cbFirstCard = i ;
			break ;
		}

		if(cbFirstCard<cbHandCardCount-1)
			AnalyseOutCardType(cbHandCardData+cbFirstCard, cbHandCardCount-cbFirstCard, CardTypeResult) ;
		else
			AnalyseOutCardType(cbHandCardData, cbHandCardCount, CardTypeResult) ;

		//计算单牌
		unsigned char cbMinSingleCardCount[4] ;
		cbMinSingleCardCount[0]=MAX_COUNT ;
		cbMinSingleCardCount[1]=MAX_COUNT ;
		cbMinSingleCardCount[2]=MAX_COUNT ;
		cbMinSingleCardCount[3]=MAX_COUNT ;
		unsigned char cbIndex[4]={0} ;
		unsigned char cbOutcardType[4]={CT_ERROR} ;
		unsigned char cbMinValue=MAX_COUNT ; 
		unsigned char cbMinSingleCountInFour=MAX_COUNT ;
		unsigned char cbMinCardType=CT_ERROR ;
		unsigned char cbMinIndex=0 ;

		//分析地主单牌
		unsigned char cbBankerSingleCardData[MAX_COUNT] ;
		unsigned char cbBankerSingleCardCount=AnalyseSinleCardCount(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], NULL, 0, cbBankerSingleCardData) ;
		unsigned char cbBankerSingleCardLogic = 0 ;
		if(cbBankerSingleCardCount>2 && GetCardLogicValue(cbBankerSingleCardData[1])<=10) cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[1]) ;
		else if(cbBankerSingleCardCount>0 && GetCardLogicValue(cbBankerSingleCardData[0])<=10) cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[0]) ;

		//除炸弹外的牌
		for(unsigned char cbCardType=CT_SINGLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType] ;
			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				//不能放走地主小牌
				if(cbCardType==CT_SINGLE && GetCardLogicValue(tmpCardResult.cbCardData[i][0])<cbBankerSingleCardLogic) continue ;

				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//搜索cbMinSingleCardCount[4]的最大值
				for(unsigned char j=0; j<4; ++j)
				{
					if(cbMinSingleCardCount[j]>=cbTmpCount)
					{
						cbMinSingleCardCount[j] = cbTmpCount ;
						cbIndex[j] = i ;
						cbOutcardType[j] = cbCardType ;
						break ;
					}
				}

				//保存最小值
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//最小牌型
					cbMinCardType = cbCardType ;
					//最小牌型中的最小单牌
					cbMinSingleCountInFour=cbTmpCount ;						
					//最小牌型中的最小牌
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//选择最小牌
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//对王加一只
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//对王
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//设置变量
				OutCardResult.cbCardCount=CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[cbMinCardType].cbCardData[Index],CardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}
		//如果只剩炸弹
		else
		{	
			unsigned char Index=0 ;
			unsigned char cbLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			//最小炸弹
			for(unsigned char i=0; i<CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
				if(cbLogicCardValue>GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
				{
					cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]) ;
					Index = i ;
				}

				//设置变量
				OutCardResult.cbCardCount=CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[CT_BOMB_CARD].cbCardData[Index],CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index]*sizeof(unsigned char));

				return ;
		}

		//如果都没有搜索到就出最小的一张
		OutCardResult.cbCardCount = 1 ;
		OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount-1] ;
		return ;
}

//地主上家（后出牌）
void CAndroidAI::UpsideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult) 
{

	//零下标没用
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//初始变量
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//出牌类型
	unsigned char cbOutCardType = GetCardType(cbTurnCardData, cbTurnCardCount) ;

	//搜索可出牌
	tagOutCardTypeResult BankerOutCardTypeResult[13] ;
	my_memset(BankerOutCardTypeResult, sizeof(BankerOutCardTypeResult)) ;

	AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], BankerOutCardTypeResult) ;
	AnalyseOutCardType(cbHandCardData,cbHandCardCount,cbTurnCardData,cbTurnCardCount, CardTypeResult) ;

	//只剩炸弹
	if(cbHandCardCount==CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
	{
		OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0] ;
		memcpy(OutCardResult.cbResultCard,  CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount) ;

		return ;
	}
	//双王炸弹和一手
	else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}

	//如果庄家没有此牌型了则不压对家牌
	if( m_cbUserCardCount[m_wBankerUser]<=5 && wOutCardUser!=m_wBankerUser &&
		(BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount==0 ||
		GetCardLogicValue(BankerOutCardTypeResult[cbOutCardType].cbCardData[0][0])<=GetCardLogicValue(cbTurnCardData[0])) &&
		CardTypeResult[cbOutCardType].cbEachHandCardCount[0]!=cbHandCardCount)//不能一次出完
	{
		//放弃出牌
		return ;
	}

	//下家为地主，而且地主扑克少于5张
	if(m_cbUserCardCount[m_wBankerUser]<=5 && CardTypeResult[cbOutCardType].cbCardTypeCount>0 && cbOutCardType!=CT_BOMB_CARD && 
		((GetCardLogicValue(cbTurnCardData[0])<12 && wOutCardUser!=m_wBankerUser && BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount>0) ||//对家出牌
		(wOutCardUser==m_wBankerUser)))//地主出牌
	{
		unsigned char Index=0;
		//寻找可以压住地主的最小一手牌
		unsigned char cbThisOutTypeMinSingleCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, CardTypeResult[cbOutCardType].cbCardData[0], CardTypeResult[cbOutCardType].cbEachHandCardCount[0]) ;
		unsigned char cbBestIndex = 255 ;
		for(unsigned char i=0; i<CardTypeResult[cbOutCardType].cbCardTypeCount; ++i)
		{
			unsigned char cbTmpSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, CardTypeResult[cbOutCardType].cbCardData[i], CardTypeResult[cbOutCardType].cbEachHandCardCount[i]) ;
			if((BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount>0 && 
				GetCardLogicValue(CardTypeResult[cbOutCardType].cbCardData[i][0])>=GetCardLogicValue(BankerOutCardTypeResult[cbOutCardType].cbCardData[0][0]) ||
				BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount==0) && 
				cbTmpSingleCardCount<=cbThisOutTypeMinSingleCount)
			{
				cbBestIndex = i ;
				cbThisOutTypeMinSingleCount = cbTmpSingleCardCount ;
			}

			if((BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount>0 && 
				GetCardLogicValue(CardTypeResult[cbOutCardType].cbCardData[i][0])>=GetCardLogicValue(BankerOutCardTypeResult[cbOutCardType].cbCardData[0][0]) ||
				BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount==0))
				Index = i ;
			else break ;
		}

		if(cbBestIndex!=255)
		{
			OutCardResult.cbCardCount = CardTypeResult[cbOutCardType].cbEachHandCardCount[cbBestIndex] ;
			memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[cbBestIndex], OutCardResult.cbCardCount) ;
		}
		else
		{
			OutCardResult.cbCardCount = CardTypeResult[cbOutCardType].cbEachHandCardCount[Index] ;
			memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index], OutCardResult.cbCardCount) ;
		}

		return ;
	}

	//取出四个最小单牌
	unsigned char cbMinSingleCardCount[4] ;
	cbMinSingleCardCount[0]=MAX_COUNT ;
	cbMinSingleCardCount[1]=MAX_COUNT ;
	cbMinSingleCardCount[2]=MAX_COUNT ;
	cbMinSingleCardCount[3]=MAX_COUNT ;
	unsigned char cbIndex[4]={0} ;	
	unsigned char cbMinSingleCountInFour=MAX_COUNT ;

	//可出扑克（这里已经过滤掉炸弹了）
	tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType] ;

	for(unsigned char i=0; i<CanOutCard.cbCardTypeCount; ++i)
	{
		//最小单牌
		unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,CanOutCard.cbCardData[i], CanOutCard.cbEachHandCardCount[i]) ; 
		unsigned char cbMaxValue=0 ; 
		unsigned char Index = 0 ;

		//搜索cbMinSingleCardCount[4]的最大值
		for(unsigned char j=0; j<4; ++j)
		{
			if(cbMinSingleCardCount[j]>=cbTmpCount)
			{
				cbMinSingleCardCount[j] = cbTmpCount ;
				cbIndex[j] = i ;
				break ;
			}
		}

	}

	for(unsigned char i=0; i<4; ++i)
		if(cbMinSingleCountInFour>cbMinSingleCardCount[i]) cbMinSingleCountInFour = cbMinSingleCardCount[i] ;


	//原始单牌数
	unsigned char cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,NULL,0) ;

	//朋友出牌
	bool bFriendOut = m_wBankerUser!=wOutCardUser ;
	if(bFriendOut)
	{
		if(CanOutCard.cbCardTypeCount>0)
		{
			//分析地主单牌
			unsigned char cbBankerSingleCardData[MAX_COUNT] ;
			unsigned char cbBankerSingleCardCount=AnalyseSinleCardCount(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], NULL, 0, cbBankerSingleCardData) ;
			unsigned char cbBankerSingleCardLogic = 0 ;
			if(cbBankerSingleCardCount>2 && GetCardLogicValue(cbBankerSingleCardData[1])<=10) cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[1]) ;
			else if(cbBankerSingleCardCount>0 && GetCardLogicValue(cbBankerSingleCardData[0])<=10) cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[0]) ;

			unsigned char cbMinLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			bool bFindCard = false ;
			unsigned char cbCanOutIndex=0 ;
			for(unsigned char i=0; i<4; ++i)
			{
				unsigned char Index = cbIndex[i] ;

				bool bCanOut = false ;
				if(cbOutCardType==CT_SINGLE && GetCardLogicValue(cbTurnCardData[0])<cbBankerSingleCardLogic &&
					GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=14 && cbMinSingleCardCount[i]<cbOriginSingleCardCount+2)
					bCanOut = true ;

				//小于J的牌，或者小于K而且是散牌
				if(bCanOut ||
					((cbMinSingleCardCount[i]<cbOriginSingleCardCount+4 && cbMinSingleCardCount[i]<=cbMinSingleCountInFour && 
					(GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=11 || (cbMinSingleCardCount[i]<cbOriginSingleCardCount)&&GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=13)) &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]) && cbHandCardCount>5))
				{
					//搜索有没有大牌（针对飞机带翅膀后面的带牌）
					bool bNoLargeCard = true ;
					for(unsigned char k=3; k<CanOutCard.cbEachHandCardCount[Index]; ++k)
					{
						//有大牌而且不能一次出完
						if(GetCardLogicValue(CanOutCard.cbCardData[Index][k])>=15 && 
							CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount) bNoLargeCard = false ;
					}
					if(bNoLargeCard)
					{
						bFindCard = true ;
						cbCanOutIndex = Index ; 
						cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]) ;
					}
				}
				else if(cbHandCardCount<5 && cbMinSingleCardCount[i]<cbOriginSingleCardCount+4 && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
				{
					bFindCard = true ;
					cbCanOutIndex = Index ; 
					cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]) ;
				}
			}

			if(bFindCard)
			{

				//设置变量
				OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
				memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

				return ;
			}
			//手上少于五张牌
			else if(cbHandCardCount<=5)
			{
				unsigned char cbMinLogicCard = GetCardLogicValue(0x4f)+1 ;
				unsigned char cbCanOutIndex = 0 ;
				for(unsigned char i=0; i<4; ++i)
					if(cbMinSingleCardCount[i]<MAX_COUNT && cbMinSingleCardCount[i]<=cbMinSingleCountInFour && cbMinLogicCard>GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) && 
						GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0])<=14)
					{
						cbMinLogicCard = GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) ;
						cbCanOutIndex = cbIndex[i] ;
					}

					if(cbMinLogicCard != (GetCardLogicValue(0x4f)+1))
					{
						//设置变量
						OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
						memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

						return ;
					}
			}

			return ;
		}
		else
		{
			return ;
		}

	}
	//地主出牌
	else
	{
		if(CanOutCard.cbCardTypeCount>0)
		{
			unsigned char cbMinLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			bool bFindCard = false ;
			unsigned char cbCanOutIndex=0 ;
			for(unsigned char i=0; i<4; ++i)
			{
				unsigned char Index = cbIndex[i] ;

				if((cbMinSingleCardCount[i]<cbOriginSingleCardCount+4)  && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
				{
					//针对大牌
					bool bNoLargeCard = true ;

					//当地主手上牌数大于4，而且地主出的是小于K的牌而且不是地主手上的最大牌时，不能出2去打
					if(m_cbUserCardCount[m_wBankerUser]>=4 && cbHandCardCount>=5  && CanOutCard.cbEachHandCardCount[Index]>=2 && 
						GetCardLogicValue(CanOutCard.cbCardData[Index][0])>=15 &&
						GetCardLogicValue(cbTurnCardData[0])<13 &&
						GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) && 
						CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
						bNoLargeCard=false ;

					//搜索有没有大牌（针对飞机带翅膀后面的带牌）
					for(unsigned char k=3; k<CanOutCard.cbEachHandCardCount[Index]; ++k)
					{
						if(GetCardLogicValue(CanOutCard.cbCardData[Index][k])>=15 && 
							CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
							bNoLargeCard = false ;
					}
					if(bNoLargeCard)
					{
						bFindCard = true ;
						cbCanOutIndex = Index ; 
						cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]) ;
					}
				}
			}

			if(bFindCard)
			{
				//地主的最大牌
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;
				bool bCanOut=true ;

				//王只压2
				if(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard)
				{
					if(GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0])==0x40 && GetCardLogicValue(cbTurnCardData[0])<=14 && cbHandCardCount>5) 								
					{
						bCanOut = false ;
					}
				}

				if(bCanOut)
				{
					//设置变量
					OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
					memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

					return ;
				}
			}

			if(cbOutCardType==CT_SINGLE)
			{
				//地主的最大牌
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;

				if(GetCardLogicValue(cbTurnCardData[0])==14 || 
					GetCardLogicValue(cbTurnCardData[0])>=cbLargestLogicCard || 
					(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard-1) ||
					m_cbUserCardCount[m_wBankerUser]<=5)
				{
					//取一张大于等于2而且要比地主出的牌大的牌，
					unsigned char cbIndex=MAX_COUNT ;
					for(unsigned char i=0; i<cbHandCardCount; ++i)
						if(GetCardLogicValue(cbHandCardData[i])>GetCardLogicValue(cbTurnCardData[0]) &&	GetCardLogicValue(cbHandCardData[i])>=15)
						{
							cbIndex = i ;
						}
						if(cbIndex!=MAX_COUNT)
						{
							//设置变量
							OutCardResult.cbCardCount=1;
							OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex] ;

							return ;
						}
				}
			}
		}

		//还要考虑炸弹
		if(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount>0 && cbHandCardCount<=10)
		{
			tagOutCardTypeResult const &BomCard = CardTypeResult[CT_BOMB_CARD] ;
			unsigned char cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[0][0]) ;
			unsigned char Index = 0 ;
			for(unsigned char i=0; i<BomCard.cbCardTypeCount; ++i)
			{
				if(cbMinLogicValue>GetCardLogicValue(BomCard.cbCardData[i][0]))
				{
					cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[i][0]) ;
					Index = i ;
				}
			}

			//判断出了炸弹后的单牌数
			unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]) ;
			if(cbSingleCardCount>=3 || (cbOutCardType==CT_SINGLE && GetCardLogicValue(cbTurnCardData[0])<15)) return ;

			//设置变量
			OutCardResult.cbCardCount=BomCard.cbEachHandCardCount[Index];
			memcpy(OutCardResult.cbResultCard,BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]*sizeof(unsigned char));

			return ;
		}

		return ;
	}
	return ;
}

//地主下家（先出牌）
void CAndroidAI::UndersideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wMeChairID,tagOutCardResult & OutCardResult) 
{
	//零下标没用
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//初始变量
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	unsigned char cbLineCard[MAX_COUNT] ;
	unsigned char cbThreeLineCard[MAX_COUNT] ;
	unsigned char cbDoubleLineCard[MAX_COUNT] ;
	unsigned char cbLineCardCount;
	unsigned char cbThreeLineCardCount ;
	unsigned char cbDoubleLineCount ;
	GetAllLineCard(cbHandCardData, cbHandCardCount, cbLineCard, cbLineCardCount) ;
	GetAllThreeCard(cbHandCardData, cbHandCardCount, cbThreeLineCard, cbThreeLineCardCount) ;
	GetAllDoubleCard(cbHandCardData, cbHandCardCount, cbDoubleLineCard, cbDoubleLineCount) ;

	//如果有顺牌和单只或一对，而且单只或对比地主的小，则先出顺
	{
		if(cbLineCardCount+1==cbHandCardCount && CT_SINGLE==GetCardType(cbLineCard, cbLineCardCount))
		{
			OutCardResult.cbCardCount = cbLineCardCount ;
			memcpy(OutCardResult.cbResultCard, cbLineCard, cbLineCardCount) ;
		}
		else if(cbThreeLineCardCount+1==cbHandCardCount && CT_THREE_LINE==GetCardType(cbThreeLineCard, cbThreeLineCardCount))
		{
			OutCardResult.cbCardCount = cbThreeLineCardCount ;
			memcpy(OutCardResult.cbResultCard, cbThreeLineCard, cbThreeLineCardCount) ;
		}
		else if(cbDoubleLineCount+1==cbHandCardCount && CT_DOUBLE_LINE==GetCardType(cbDoubleLineCard, cbDoubleLineCount))
		{
			OutCardResult.cbCardCount = cbDoubleLineCount ;
			memcpy(OutCardResult.cbResultCard, cbDoubleLineCard, cbDoubleLineCount) ;
		}
		//双王炸弹和一手
		else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
		{
			OutCardResult.cbCardCount = 2 ;
			OutCardResult.cbResultCard[0] = 0x4f ;
			OutCardResult.cbResultCard[1] = 0x4e ;
		}

		if(OutCardResult.cbCardCount>0)
		{
			return ;
		}
	}
	//对王加一只
	if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//对王
	else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//只剩一手牌
	else if(CT_ERROR!=GetCardType(cbHandCardData, cbHandCardCount))
	{
		OutCardResult.cbCardCount = cbHandCardCount ;
		memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount) ;
		return ;
	}

	//只剩一张和一手
	if(cbHandCardCount>=2)
	{
		//地主扑克
		tagOutCardTypeResult BankerCanOutCardType1[13] ;
		my_memset(BankerCanOutCardType1, sizeof(BankerCanOutCardType1)) ;
		tagOutCardTypeResult BankerCanOutCardType2[13] ;
		my_memset(BankerCanOutCardType2, sizeof(BankerCanOutCardType2)) ;

		unsigned char cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount-1) ;
		unsigned char cbSecondHandCardType = GetCardType(cbHandCardData+1, cbHandCardCount-1) ;

		//地主可以出的牌
		if(cbFirstHandCardType!=CT_ERROR)
			AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], cbHandCardData, cbHandCardCount-1, BankerCanOutCardType1) ;
		if(cbSecondHandCardType!=CT_ERROR)
			AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], cbHandCardData+1, cbHandCardCount-1, BankerCanOutCardType2) ;

		if(cbSecondHandCardType!=CT_ERROR && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO && 
			BankerCanOutCardType2[cbSecondHandCardType].cbCardTypeCount==0 && BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData+1, cbHandCardCount-1) ;
			return ;
		}

		if(cbFirstHandCardType!=CT_ERROR && cbFirstHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbFirstHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			BankerCanOutCardType1[cbFirstHandCardType].cbCardTypeCount==0 && BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount-1) ;
			return ;
		}

		if(GetCardLogicValue(cbHandCardData[0])>=GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) &&
			CT_ERROR!=cbSecondHandCardType && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = 1 ;
			OutCardResult.cbResultCard[0] = cbHandCardData[0] ;
			return ;
		}

		if(CT_ERROR!=cbSecondHandCardType && cbSecondHandCardType!=CT_FOUR_LINE_TAKE_ONE && cbSecondHandCardType!= CT_FOUR_LINE_TAKE_TWO &&
			BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount==0)
		{
			OutCardResult.cbCardCount = cbHandCardCount-1 ;
			memcpy(OutCardResult.cbResultCard, cbHandCardData+1, cbHandCardCount-1) ;
			return ;
		}
	}


	//下家为地主，而且地主扑克少于5张
	//	if(m_cbUserCardCount[m_wBankerUser]<=5)
	{
		//分析扑克
		tagOutCardTypeResult MeOutCardTypeResult[13] ;
		my_memset(MeOutCardTypeResult, sizeof(MeOutCardTypeResult)) ;
		AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;

		//对家扑克
		int wFriendID ;
		for(int wChairID=0; wChairID<GAME_PLAYER; ++wChairID) 
			if(wChairID!=m_wBankerUser && wMeChairID!=wChairID) wFriendID = wChairID ;

		/*
		unsigned char cbFirstCard=0 ;
		//过滤王和2
		for(unsigned char i=0; i<cbHandCardCount; ++i) 
		if(GetCardLogicValue(cbHandCardData[i])<15)
		{
		cbFirstCard = i ;
		break ;
		}


		if(cbFirstCard<cbHandCardCount-1)
		AnalyseOutCardType(cbHandCardData+cbFirstCard, cbHandCardCount-cbFirstCard, MeOutCardTypeResult) ;
		else
		AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;*/


		//计算单牌
		unsigned char cbMinSingleCardCount[4] ;
		cbMinSingleCardCount[0]=MAX_COUNT ;
		cbMinSingleCardCount[1]=MAX_COUNT ;
		cbMinSingleCardCount[2]=MAX_COUNT ;
		cbMinSingleCardCount[3]=MAX_COUNT ;
		unsigned char cbIndex[4]={0} ;
		unsigned char cbOutcardType[4]={CT_ERROR} ;
		unsigned char cbMinValue=MAX_COUNT ; 
		unsigned char cbMinSingleCountInFour=MAX_COUNT ;
		unsigned char cbMinCardType=CT_ERROR ;
		unsigned char cbMinIndex=0 ;

		//除炸弹外的牌
		for(unsigned char cbCardType=CT_DOUBLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType] ;

			//相同牌型，相同长度，单连，对连等相同牌型可能长度不一样
			unsigned char cbThisHandCardCount = MAX_COUNT ;

			//地主扑克
			tagOutCardTypeResult BankerCanOutCard[13] ;
			my_memset(BankerCanOutCard, sizeof(BankerCanOutCard)) ;

			tagOutCardTypeResult FriendOutCardTypeResult[13] ;
			my_memset(FriendOutCardTypeResult, sizeof(FriendOutCardTypeResult)) ;

			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				//重新分析
				if(tmpCardResult.cbEachHandCardCount[i]!=cbThisHandCardCount)
				{
					AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], 
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,BankerCanOutCard) ;
					AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID],
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,FriendOutCardTypeResult) ;
				}

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//地主可以压牌，而且队友不可以压地主
				if((BankerCanOutCard[cbCardType].cbCardTypeCount>0&&FriendOutCardTypeResult[cbCardType].cbCardTypeCount==0) || (BankerCanOutCard[cbCardType].cbCardTypeCount>0 && FriendOutCardTypeResult[cbCardType].cbCardTypeCount>0 &&
					GetCardLogicValue(FriendOutCardTypeResult[cbCardType].cbCardData[0][0])<=GetCardLogicValue(BankerCanOutCard[cbCardType].cbCardData[0][0])))
				{
					continue ;
				}
				//是否有大牌
				if(tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
				{
					bool bHaveLargeCard=false ;
					for(unsigned char j=0; j<tmpCardResult.cbEachHandCardCount[i]; ++j)
						if(GetCardLogicValue(tmpCardResult.cbCardData[i][j])>=15) bHaveLargeCard=true ;

					if(cbCardType!=CT_SINGLE_LINE && cbCardType!=CT_DOUBLE_LINE && GetCardLogicValue(tmpCardResult.cbCardData[i][0])==14) bHaveLargeCard=true ; 

					if(bHaveLargeCard) continue ;
				}

				//地主是否可以走掉，这里都没有考虑炸弹
				if(tmpCardResult.cbEachHandCardCount[i]==m_cbUserCardCount[m_wBankerUser] &&
					GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0])>GetCardLogicValue(tmpCardResult.cbCardData[i][0])) continue ;

				//搜索cbMinSingleCardCount[4]的最大值
				for(unsigned char j=0; j<4; ++j)
				{
					if(cbMinSingleCardCount[j]>=cbTmpCount)
					{
						cbMinSingleCardCount[j] = cbTmpCount ;
						cbIndex[j] = i ;
						cbOutcardType[j] = cbCardType ;
						break ;
					}
				}

				//保存最小值
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//最小牌型
					cbMinCardType = cbCardType ;
					//最小牌型中的最小单牌
					cbMinSingleCountInFour=cbTmpCount ;						
					//最小牌型中的最小牌
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour>=AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0)+3 && 
			m_cbUserCardCount[m_wBankerUser]>4)
			cbMinSingleCountInFour=MAX_COUNT ;

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//选择最小牌
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//对王加一只
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//对王
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//设置变量
				OutCardResult.cbCardCount=MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,MeOutCardTypeResult[cbMinCardType].cbCardData[Index],MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}

		//如果地主扑克少于5，还没有找到适合的牌则从大出到小
		if(OutCardResult.cbCardCount<=0 && m_cbUserCardCount[m_wBankerUser]<=5)
		{
			//只有一张牌时不能放地主走
			if(m_cbUserCardCount[m_wBankerUser]==1 && MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount>0)
			{
				unsigned char Index=MAX_COUNT ;
				for(unsigned char i=0; i<MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount; ++i)
				{
					if(GetCardLogicValue(MeOutCardTypeResult[CT_SINGLE].cbCardData[i][0])>=GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]))
					{
						Index=i ;
					}
					else break ;
				}

				if(MAX_COUNT!=Index)
				{
					OutCardResult.cbCardCount = MeOutCardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] ;
					memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[CT_SINGLE].cbCardData[Index], OutCardResult.cbCardCount) ;
					return ;
				}
			}
		}
	}

	unsigned char cbFirstCard=0 ;
	//过滤王和2
	for(unsigned char i=0; i<cbHandCardCount; ++i) 
		if(GetCardLogicValue(cbHandCardData[i])<15)
		{
			cbFirstCard = i ;
			break ;
		}

		if(cbFirstCard<cbHandCardCount-1)
			AnalyseOutCardType(cbHandCardData+cbFirstCard, cbHandCardCount-cbFirstCard, CardTypeResult) ;
		else
			AnalyseOutCardType(cbHandCardData, cbHandCardCount, CardTypeResult) ;

		//计算单牌
		unsigned char cbMinSingleCardCount[4] ;
		cbMinSingleCardCount[0]=MAX_COUNT ;
		cbMinSingleCardCount[1]=MAX_COUNT ;
		cbMinSingleCardCount[2]=MAX_COUNT ;
		cbMinSingleCardCount[3]=MAX_COUNT ;
		unsigned char cbIndex[4]={0} ;
		unsigned char cbOutcardType[4]={CT_ERROR} ;
		unsigned char cbMinValue=MAX_COUNT ; 
		unsigned char cbMinSingleCountInFour=MAX_COUNT ;
		unsigned char cbMinCardType=CT_ERROR ;
		unsigned char cbMinIndex=0 ;

		//除炸弹外的牌
		for(unsigned char cbCardType=CT_SINGLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType] ;
			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//搜索cbMinSingleCardCount[4]的最大值
				for(unsigned char j=0; j<4; ++j)
				{
					if(cbMinSingleCardCount[j]>=cbTmpCount)
					{
						cbMinSingleCardCount[j] = cbTmpCount ;
						cbIndex[j] = i ;
						cbOutcardType[j] = cbCardType ;
						break ;
					}
				}

				//保存最小值
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//最小牌型
					cbMinCardType = cbCardType ;
					//最小牌型中的最小单牌
					cbMinSingleCountInFour=cbTmpCount ;						
					//最小牌型中的最小牌
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//选择最小牌
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//对王加一只
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//对王
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//设置变量
				OutCardResult.cbCardCount=CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[cbMinCardType].cbCardData[Index],CardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}
		//如果只剩炸弹
		else
		{	
			unsigned char Index=0 ;
			unsigned char cbLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			//最小炸弹
			for(unsigned char i=0; i<CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
				if(cbLogicCardValue>GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
				{
					cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]) ;
					Index = i ;
				}

				//设置变量
				OutCardResult.cbCardCount=CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[CT_BOMB_CARD].cbCardData[Index],CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index]*sizeof(unsigned char));

				return ;
		}

		//如果都没有搜索到就出最小的一张
		OutCardResult.cbCardCount = 1 ;
		OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount-1] ;
		return ;
}
//地主下家（后出牌）
void CAndroidAI::UndersideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult)
{
	//初始变量
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//零下标没用
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//出牌类型
	unsigned char cbOutCardType = GetCardType(cbTurnCardData,cbTurnCardCount) ;

	//搜索可出牌
	tagOutCardTypeResult BankerOutCardTypeResult[13] ;
	my_memset(BankerOutCardTypeResult, sizeof(BankerOutCardTypeResult)) ;

	AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], BankerOutCardTypeResult) ;
	AnalyseOutCardType(cbHandCardData,cbHandCardCount,cbTurnCardData,cbTurnCardCount, CardTypeResult) ;

	//只剩炸弹
	if(cbHandCardCount==CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
	{
		OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0] ;
		memcpy(OutCardResult.cbResultCard,  CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount) ;

		return ;
	}
	//双王炸弹和一手
	else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}

	//取出四个最小单牌
	unsigned char cbMinSingleCardCount[4] ;
	cbMinSingleCardCount[0]=MAX_COUNT ;
	cbMinSingleCardCount[1]=MAX_COUNT ;
	cbMinSingleCardCount[2]=MAX_COUNT ;
	cbMinSingleCardCount[3]=MAX_COUNT ;
	unsigned char cbIndex[4]={0} ;	
	unsigned char cbMinSingleCountInFour=MAX_COUNT ;

	//可出扑克（这里已经过滤掉炸弹了）
	tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType] ;

	for(unsigned char i=0; i<CanOutCard.cbCardTypeCount; ++i)
	{
		//最小单牌
		unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,CanOutCard.cbCardData[i], CanOutCard.cbEachHandCardCount[i]) ; 
		unsigned char cbMaxValue=0 ; 
		unsigned char Index = 0 ;

		//搜索cbMinSingleCardCount[4]的最大值
		for(unsigned char j=0; j<4; ++j)
		{
			if(cbMinSingleCardCount[j]>=cbTmpCount)
			{
				cbMinSingleCardCount[j] = cbTmpCount ;
				cbIndex[j] = i ;
				break ;
			}
		}

	}

	for(unsigned char i=0; i<4; ++i)
		if(cbMinSingleCountInFour>cbMinSingleCardCount[i]) cbMinSingleCountInFour = cbMinSingleCardCount[i] ;


	//原始单牌数
	unsigned char cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,NULL,0) ;

	//朋友出牌
	bool bFriendOut = m_wBankerUser!=wOutCardUser ;
	if(bFriendOut)
	{
		if(CanOutCard.cbCardTypeCount>0)
		{
			unsigned char cbMinLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			bool bFindCard = false ;
			unsigned char cbCanOutIndex=0 ;
			for(unsigned char i=0; i<4; ++i)
			{
				unsigned char Index = cbIndex[i] ;
				//小于J的牌，或者小于K而且是散牌
				if((cbMinSingleCardCount[i]<cbOriginSingleCardCount+4 && cbMinSingleCardCount[i]<=cbMinSingleCountInFour && 
					(GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=11 || (cbMinSingleCardCount[i]<cbOriginSingleCardCount)&&GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=13)) &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]) && cbHandCardCount>5)
				{
					//搜索有没有大牌（针对飞机带翅膀后面的带牌）
					bool bNoLargeCard = true ;
					for(unsigned char k=3; k<CanOutCard.cbEachHandCardCount[Index]; ++k)
					{
						//有大牌而且不能一次出完
						if(GetCardLogicValue(CanOutCard.cbCardData[Index][k])>=15 && 
							CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount) bNoLargeCard = false ;
					}
					if(bNoLargeCard)
					{
						bFindCard = true ;
						cbCanOutIndex = Index ; 
						cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]) ;
					}
				}
				else if(cbHandCardCount<5 && cbMinSingleCardCount[i]<cbOriginSingleCardCount+4 && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
				{
					bFindCard = true ;
					cbCanOutIndex = Index ; 
					cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]) ;
				}
			}

			if(bFindCard)
			{

				//设置变量
				OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
				memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

				return ;
			}
			//手上少于五张牌
			else if(cbHandCardCount<=5)
			{
				unsigned char cbMinLogicCard = GetCardLogicValue(0x4f)+1 ;
				unsigned char cbCanOutIndex = 0 ;
				for(unsigned char i=0; i<4; ++i)
					if(cbMinSingleCardCount[i]<MAX_COUNT && cbMinSingleCardCount[i]<=cbMinSingleCountInFour && cbMinLogicCard>GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) && 
						GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0])<=14)
					{
						cbMinLogicCard = GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) ;
						cbCanOutIndex = cbIndex[i] ;
					}

					if(cbMinLogicCard != (GetCardLogicValue(0x4f)+1))
					{
						//设置变量
						OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
						memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

						return ;
					}
			}

			return ;
		}
		else
		{
			return ;
		}

	}
	//地主出牌
	else
	{
		if(CanOutCard.cbCardTypeCount>0)
		{
			unsigned char cbMinLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			bool bFindCard = false ;
			unsigned char cbCanOutIndex=0 ;
			for(unsigned char i=0; i<4; ++i)
			{
				unsigned char Index = cbIndex[i] ;

				if((cbMinSingleCardCount[i]<cbOriginSingleCardCount+4)  && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
				{
					//针对大牌
					bool bNoLargeCard = true ;

					//当地主手上牌数大于4，而且地主出的是小于K的牌而且不是地主手上的最大牌时，不能出2去打
					if(m_cbUserCardCount[m_wBankerUser]>=4 && cbHandCardCount>=5 && CanOutCard.cbEachHandCardCount[Index]>=2 && 
						GetCardLogicValue(CanOutCard.cbCardData[Index][0])>=15 &&
						GetCardLogicValue(cbTurnCardData[0])<13 &&
						GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) && 
						CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
						bNoLargeCard=false ;

					//搜索有没有大牌（针对飞机带翅膀后面的带牌）
					for(unsigned char k=3; k<CanOutCard.cbEachHandCardCount[Index]; ++k)
					{
						if(GetCardLogicValue(CanOutCard.cbCardData[Index][k])>=15 && 
							CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
							bNoLargeCard = false ;
					}
					if(bNoLargeCard)
					{
						bFindCard = true ;
						cbCanOutIndex = Index ; 
						cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]) ;
					}
				}
			}

			if(bFindCard)
			{
				//地主的最大牌
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;
				bool bCanOut=true ;

				//王只压2
				if(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard)
				{
					if(GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0])==0x40 && GetCardLogicValue(cbTurnCardData[0])<=14 && cbHandCardCount>5) 								
					{
						bCanOut = false ;
					}
				}

				if(bCanOut)
				{
					//设置变量
					OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
					memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

					return ;
				}
			}

			if(cbOutCardType==CT_SINGLE)
			{
				//地主的最大牌
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;

				if(GetCardLogicValue(cbTurnCardData[0])==14 || 
					GetCardLogicValue(cbTurnCardData[0])>=cbLargestLogicCard || 
					(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard-1) ||
					m_cbUserCardCount[m_wBankerUser]<=5)
				{
					//取一张大于等于2而且要比地主出的牌大的牌，
					unsigned char cbIndex=MAX_COUNT ;
					for(unsigned char i=0; i<cbHandCardCount; ++i)
						if(GetCardLogicValue(cbHandCardData[i])>GetCardLogicValue(cbTurnCardData[0]) &&
							GetCardLogicValue(cbHandCardData[i])>=15)
						{
							cbIndex = i ;
						}
						if(cbIndex!=MAX_COUNT)
						{
							//设置变量
							OutCardResult.cbCardCount=1;
							OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex] ;

							return ;
						}
				}
			}

			//当朋友不能拦截地主时
			int wMeChairID = (m_wBankerUser+1)%GAME_PLAYER ;
			int wFriendID = (wMeChairID+1)%GAME_PLAYER ;

			tagOutCardTypeResult FriendCardTypeResult[13] ;
			my_memset(FriendCardTypeResult, sizeof(FriendCardTypeResult)) ;
			AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID], cbTurnCardData, cbTurnCardCount, FriendCardTypeResult) ;

			//当朋友不能拦截地主时
			if(m_cbUserCardCount[m_wBankerUser]<=4 && FriendCardTypeResult[cbOutCardType].cbCardTypeCount==0 && CardTypeResult[cbOutCardType].cbCardTypeCount>0)
			{
				unsigned char cbMinSingleCount=MAX_COUNT ;
				unsigned char Index=0 ;
				for(unsigned char i=0; i<CardTypeResult[cbOutCardType].cbCardTypeCount; ++i)
				{
					unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, CardTypeResult[cbOutCardType].cbCardData[i], CardTypeResult[cbOutCardType].cbEachHandCardCount[i]) ;
					if(cbMinSingleCount>=cbTmpCount)
					{
						cbMinSingleCount = cbTmpCount ;
						Index = i ;
					}
				}
				//设置变量
				OutCardResult.cbCardCount=CardTypeResult[cbOutCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index], OutCardResult.cbCardCount) ;

				return ;
			}
		}

		//还要考虑炸弹
		if(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount>0 && cbHandCardCount<=10)
		{
			tagOutCardTypeResult const &BomCard = CardTypeResult[CT_BOMB_CARD] ;
			unsigned char cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[0][0]) ;
			unsigned char Index = 0 ;
			for(unsigned char i=0; i<BomCard.cbCardTypeCount; ++i)
			{
				if(cbMinLogicValue>GetCardLogicValue(BomCard.cbCardData[i][0]))
				{
					cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[i][0]) ;
					Index = i ;
				}
			}

			//判断出了炸弹后的单牌数
			unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, BomCard.cbCardData[Index], BomCard.cbEachHandCardCount[Index]) ;
			if(cbSingleCardCount>=3 || (cbOutCardType==CT_SINGLE && GetCardLogicValue(cbTurnCardData[0])<15)) return ;

			//设置变量
			OutCardResult.cbCardCount=BomCard.cbEachHandCardCount[Index];
			memcpy(OutCardResult.cbResultCard,BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]*sizeof(unsigned char));

			return ;
		}

		return ;
	}
	return ;
}

void CAndroidAI::onGameStart(PokerData* data)
{
	//所有扑克
	for (int wChairID = 0; wChairID < GAME_PLAYER; ++wChairID){
		SetUserCard(wChairID, &data->cardData[wChairID * NORMAL_COUNT], NORMAL_COUNT);
	}
}
