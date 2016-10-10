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

//��ȡ����
unsigned char CAndroidAI::GetCardType(const unsigned char cbCardData[], unsigned char cbCardCount)
{
	//������
	switch (cbCardCount)
	{
	case 0:	//����
		{
			return CT_ERROR;
		}
	case 1: //����
		{
			return CT_SINGLE;
		}
	case 2:	//���ƻ��
		{
			//�����ж�
			if ((cbCardData[0]==0x4F)&&(cbCardData[1]==0x4E)) return CT_MISSILE_CARD;
			if ((cbCardData[1] == 0x4F) && (cbCardData[0] == 0x4E)) return CT_MISSILE_CARD;

			if (GetCardLogicValue(cbCardData[0])==GetCardLogicValue(cbCardData[1])) return CT_DOUBLE;

			return CT_ERROR;
		}
	}

	//�����˿�
	tagAnalyseResult AnalyseResult;
	AnalysebCardData(cbCardData,cbCardCount,AnalyseResult);

	//�����ж�
	if (AnalyseResult.cbBlockCount[3]>0)
	{
		//�����ж�
		if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==4)) return CT_BOMB_CARD;
		if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==6)) return CT_FOUR_LINE_TAKE_ONE;
		if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==8)&&(AnalyseResult.cbBlockCount[1]==2)) return CT_FOUR_LINE_TAKE_TWO;

		return CT_ERROR;
	}

	//�����ж�
	if (AnalyseResult.cbBlockCount[2]>0)
	{
		//�����ж�
		if (AnalyseResult.cbBlockCount[2]>1)
		{
			//��������
			unsigned char cbCardData=AnalyseResult.cbCardData[2][0];
			unsigned char cbFirstLogicValue=GetCardLogicValue(cbCardData);

			//�������
			if (cbFirstLogicValue>=15) return CT_ERROR;

			//�����ж�
			for (unsigned char i=1;i<AnalyseResult.cbBlockCount[2];i++)
			{
				unsigned char cbCardData=AnalyseResult.cbCardData[2][i*3];
				if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
			}
		}
		else if( cbCardCount == 3 ) return CT_THREE;

		//�����ж�
		if (AnalyseResult.cbBlockCount[2]*3==cbCardCount) return CT_THREE_LINE;
		if (AnalyseResult.cbBlockCount[2]*4==cbCardCount) return CT_THREE_LINE_TAKE_ONE;
		if ((AnalyseResult.cbBlockCount[2]*5==cbCardCount)&&(AnalyseResult.cbBlockCount[1]==AnalyseResult.cbBlockCount[2])) return CT_THREE_LINE_TAKE_TWO;

		return CT_ERROR;
	}

	//��������
	if (AnalyseResult.cbBlockCount[1]>=3)
	{
		//��������
		unsigned char cbCardData=AnalyseResult.cbCardData[1][0];
		unsigned char cbFirstLogicValue=GetCardLogicValue(cbCardData);

		//�������
		if (cbFirstLogicValue>=15) return CT_ERROR;

		//�����ж�
		for (unsigned char i=1;i<AnalyseResult.cbBlockCount[1];i++)
		{
			unsigned char cbCardData=AnalyseResult.cbCardData[1][i*2];
			if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
		}

		//�����ж�
		if ((AnalyseResult.cbBlockCount[1]*2)==cbCardCount) return CT_DOUBLE_LINE;

		return CT_ERROR;
	}

	//�����ж�
	if ((AnalyseResult.cbBlockCount[0]>=5)&&(AnalyseResult.cbBlockCount[0]==cbCardCount))
	{
		//��������
		unsigned char cbCardData=AnalyseResult.cbCardData[0][0];
		unsigned char cbFirstLogicValue=GetCardLogicValue(cbCardData);

		//�������
		if (cbFirstLogicValue>=15) return CT_ERROR;

		//�����ж�
		for (unsigned char i=1;i<AnalyseResult.cbBlockCount[0];i++)
		{
			unsigned char cbCardData=AnalyseResult.cbCardData[0][i];
			if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
		}

		return CT_SINGLE_LINE;
	}

	return CT_ERROR;
}

//�߼���ֵ
unsigned char CAndroidAI::GetCardLogicValue(unsigned char cbCardData)
{
	//�˿�����
	unsigned char cbCardColor=GetCardColor(cbCardData);
	unsigned char cbCardValue=GetCardValue(cbCardData);


	if(cbCardValue<=0 || cbCardValue>(MASK_VALUE&0x4f))
		return 0 ;


	//ת����ֵ
	if (cbCardColor==0x40) return cbCardValue+2;
	return (cbCardValue<=2)?(cbCardValue+13):cbCardValue;
}

//��Ч�ж�
bool CAndroidAI::IsValidCard(unsigned char cbCardData)
{
	//��ȡ����
	unsigned char cbCardColor=GetCardColor(cbCardData);
	unsigned char cbCardValue=GetCardValue(cbCardData);

	//��Ч�ж�
	if ((cbCardData==0x4E)||(cbCardData==0x4F)) return true;
	if ((cbCardColor<=0x30)&&(cbCardValue>=0x01)&&(cbCardValue<=0x0D)) return true;

	return false;
}

//�Ա��˿�
bool CAndroidAI::CompareCard(const unsigned char cbFirstCard[], const unsigned char cbNextCard[], unsigned char cbFirstCount, unsigned char cbNextCount)
{
	//��ȡ����
	unsigned char cbNextType=GetCardType(cbNextCard,cbNextCount);
	unsigned char cbFirstType=GetCardType(cbFirstCard,cbFirstCount);

	//�����ж�
	if (cbNextType==CT_ERROR) return false;
	if (cbNextType==CT_MISSILE_CARD) return true;

	//ը���ж�
	if ((cbFirstType!=CT_BOMB_CARD)&&(cbNextType==CT_BOMB_CARD)) return true;
	if ((cbFirstType==CT_BOMB_CARD)&&(cbNextType!=CT_BOMB_CARD)) return false;

	//�����ж�
	if ((cbFirstType!=cbNextType)||(cbFirstCount!=cbNextCount)) return false;

	//��ʼ�Ա�
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
			//��ȡ��ֵ
			unsigned char cbNextLogicValue=GetCardLogicValue(cbNextCard[0]);
			unsigned char cbFirstLogicValue=GetCardLogicValue(cbFirstCard[0]);

			//�Ա��˿�
			return cbNextLogicValue>cbFirstLogicValue;
		}
	case CT_THREE_LINE_TAKE_ONE:
	case CT_THREE_LINE_TAKE_TWO:
		{
			//�����˿�
			tagAnalyseResult NextResult;
			tagAnalyseResult FirstResult;
			AnalysebCardData(cbNextCard,cbNextCount,NextResult);
			AnalysebCardData(cbFirstCard,cbFirstCount,FirstResult);

			//��ȡ��ֵ
			unsigned char cbNextLogicValue=GetCardLogicValue(NextResult.cbCardData[2][0]);
			unsigned char cbFirstLogicValue=GetCardLogicValue(FirstResult.cbCardData[2][0]);

			//�Ա��˿�
			return cbNextLogicValue>cbFirstLogicValue;
		}
	case CT_FOUR_LINE_TAKE_ONE:
	case CT_FOUR_LINE_TAKE_TWO:
		{
			//�����˿�
			tagAnalyseResult NextResult;
			tagAnalyseResult FirstResult;
			AnalysebCardData(cbNextCard,cbNextCount,NextResult);
			AnalysebCardData(cbFirstCard,cbFirstCount,FirstResult);

			//��ȡ��ֵ
			unsigned char cbNextLogicValue=GetCardLogicValue(NextResult.cbCardData[3][0]);
			unsigned char cbFirstLogicValue=GetCardLogicValue(FirstResult.cbCardData[3][0]);

			//�Ա��˿�
			return cbNextLogicValue>cbFirstLogicValue;
		}
	}

	return false;
}

//�������
void CAndroidAI::ClearAllCard()
{
	my_memset(m_cbAllCardData,sizeof(m_cbAllCardData));
	my_memset(m_cbUserCardCount, sizeof(m_cbUserCardCount));
	m_wBankerUser = INVALID_CHAIR;
	m_lBankerOutCardCount = 0;
}


//�з��ж�
unsigned char CAndroidAI::AnalyseLandScore(int chairIdx, unsigned char cbBackCardData[])
{
	unsigned char cbLandScoreCardData[MAX_COUNT];	//�����˿�
	memcpy(cbLandScoreCardData, m_cbAllCardData[chairIdx], NORMAL_COUNT);
	memcpy(&cbLandScoreCardData[NORMAL_COUNT], cbBackCardData, BACK_COUNT);

	//������Ŀ
	unsigned char cbLargeCardCount = 0 ;
	unsigned char Index=0 ;
	while (GetCardLogicValue(cbLandScoreCardData[Index++]) >= 15)
		++cbLargeCardCount ;

	//���Ƹ���
	unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbLandScoreCardData, sizeof(cbLandScoreCardData), NULL, 0);

	//������
	if(cbLargeCardCount >= 4 && cbSingleCardCount <= 4) 
		return 3 ;

	//�����з�
	if(cbLargeCardCount <= 2) 
		return 255 ;

	//��������
	unsigned char cbMinSingleCardCount = MAX_COUNT ;
	for(int wChairID=0 , i=0; wChairID < GAME_PLAYER; ++wChairID){
		if (wChairID != chairIdx){
			unsigned char cbTmpSingleCardCount = AnalyseSinleCardCount(m_cbAllCardData[wChairID], NORMAL_COUNT, NULL, 0);
			if (cbTmpSingleCardCount < cbMinSingleCardCount){
				cbTmpSingleCardCount = cbMinSingleCardCount;
			}
		}	
	}

	//��һ��
	if(cbLargeCardCount >= 3 && cbSingleCardCount < cbMinSingleCardCount - 3)
		return 2 ;

	//�����з�
	return 255 ;
}

//�����˿�
void CAndroidAI::SortCardList(unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbSortType)
{
	//��Ŀ����
	if (cbCardCount==0) return;
	if (cbSortType==ST_CUSTOM) return;

	//ת����ֵ
	unsigned char cbSortValue[MAX_COUNT];
	for (unsigned char i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardLogicValue(cbCardData[i]);	

	//�������
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
				//���ñ�־
				bSorted=false;

				//�˿�����
				cbSwitchData=cbCardData[i];
				cbCardData[i]=cbCardData[i+1];
				cbCardData[i+1]=cbSwitchData;

				//����Ȩλ
				cbSwitchData=cbSortValue[i];
				cbSortValue[i]=cbSortValue[i+1];
				cbSortValue[i+1]=cbSwitchData;
			}	
		}
		cbLast--;
	} while(bSorted==false);

	//��Ŀ����
	if (cbSortType==ST_COUNT)
	{
		//��������
		unsigned char cbCardIndex=0;

		//�����˿�
		tagAnalyseResult AnalyseResult;
		AnalysebCardData(&cbCardData[cbCardIndex],cbCardCount-cbCardIndex,AnalyseResult);

		//��ȡ�˿�
		for (unsigned char i=0;i<CountArray(AnalyseResult.cbBlockCount);i++)
		{
			//�����˿�
			unsigned char cbIndex=CountArray(AnalyseResult.cbBlockCount)-i-1;
			memcpy(&cbCardData[cbCardIndex],AnalyseResult.cbCardData[cbIndex],AnalyseResult.cbBlockCount[cbIndex]*(cbIndex+1)*sizeof(unsigned char));

			//��������
			cbCardIndex+=AnalyseResult.cbBlockCount[cbIndex]*(cbIndex+1)*sizeof(unsigned char);
		}
	}

	return;
}

//ɾ���˿�
bool CAndroidAI::RemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, unsigned char cbCardData[], unsigned char cbCardCount)
{
	//��������
	if(cbRemoveCount>cbCardCount)
		return false ;

	//�������
	unsigned char cbDeleteCount=0,cbTempCardData[MAX_COUNT];
	if (cbCardCount>CountArray(cbTempCardData)) return false;
	memcpy(cbTempCardData,cbCardData,cbCardCount*sizeof(cbCardData[0]));

	//�����˿�
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

	//�����˿�
	unsigned char cbCardPos=0;
	for (unsigned char i=0;i<cbCardCount;i++)
	{
		if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
	}

	return true;
}

//�����˿�
void CAndroidAI::AnalysebCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult)
{
	//���ý��
	my_memset(&AnalyseResult,sizeof(AnalyseResult));

	//�˿˷���
	for (unsigned char i=0;i<cbCardCount;i++)
	{
		//��������
		unsigned char cbSameCount=1,cbCardValueTemp=0;
		unsigned char cbLogicValue=GetCardLogicValue(cbCardData[i]);

		//����ͬ��
		for (unsigned char j=i+1;j<cbCardCount;j++)
		{
			//��ȡ�˿�
			if (GetCardLogicValue(cbCardData[j])!=cbLogicValue) break;

			//���ñ���
			cbSameCount++;
		}

		//���ý��
		unsigned char cbIndex=AnalyseResult.cbBlockCount[cbSameCount-1]++;
		for (unsigned char j=0;j<cbSameCount;j++) AnalyseResult.cbCardData[cbSameCount-1][cbIndex*cbSameCount+j]=cbCardData[i+j];

		//��������
		i+=cbSameCount-1;
	}

	return;
}

//�����˿�
void CAndroidAI::SetUserCard(int wChairID, unsigned char cbCardData[], unsigned char cbCardCount)
{
	memcpy(m_cbAllCardData[wChairID], cbCardData, cbCardCount*sizeof(unsigned char)) ;
	m_cbUserCardCount[wChairID] = cbCardCount ;

	//�����˿�
	SortCardList(m_cbAllCardData[wChairID], cbCardCount, ST_ORDER) ;
}

//���õ���
void CAndroidAI::SetBackCard(int wChairID, unsigned char cbBackCardData[], unsigned char cbCardCount)
{
	unsigned char cbTmpCount = m_cbUserCardCount[wChairID] ;
	memcpy(m_cbAllCardData[wChairID]+cbTmpCount, cbBackCardData, cbCardCount*sizeof(unsigned char)) ;
	m_cbUserCardCount[wChairID] += cbCardCount ;

	//�����˿�
	SortCardList(m_cbAllCardData[wChairID], m_cbUserCardCount[wChairID], ST_ORDER) ;
}

//����ׯ��
void CAndroidAI::SetBanker(int wBanker) 
{
	m_wBankerUser = wBanker ;
}
//�����˿�
#if 0
void CAndroidAI::SetLandScoreCardData(unsigned char cbCardData[], unsigned char cbCardCount) 
{
	if(cbCardCount!=MAX_COUNT) return ;

	memcpy(m_cbLandScoreCardData, cbCardData, cbCardCount*sizeof(unsigned char)) ;
	//�����˿�
	SortCardList(m_cbLandScoreCardData, cbCardCount, ST_ORDER) ;
}
#endif

//ɾ���˿�
void CAndroidAI::RemoveUserCardData(int wChairID, unsigned char cbRemoveCardData[], unsigned char cbRemoveCardCount) 
{
	bool bSuccess = RemoveCard(cbRemoveCardData, cbRemoveCardCount, m_cbAllCardData[wChairID], m_cbUserCardCount[wChairID]) ;
	m_cbUserCardCount[wChairID] -= cbRemoveCardCount;
}

////����ΪAI����

//��������
bool CAndroidAI::SearchOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, int wOutCardUser, int wMeChairID, tagOutCardResult & OutCardResult)
{
	//����ж�
	int wUndersideOfBanker = (m_wBankerUser+1)%GAME_PLAYER ;	//�����¼�
	int wUpsideOfBanker = (wUndersideOfBanker+1)%GAME_PLAYER ;	//�����ϼ�

	//��ʼ����
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//�ȳ���
	if(cbTurnCardCount==0)
	{
		//��������
		if(wMeChairID==m_wBankerUser) BankerOutCard(cbHandCardData, cbHandCardCount, OutCardResult) ;
		//�����¼�
		else if(wMeChairID==wUndersideOfBanker) UndersideOfBankerOutCard(cbHandCardData, cbHandCardCount,wMeChairID,  OutCardResult) ;
		//�����ϼ�
		else if(wMeChairID==wUpsideOfBanker) UpsideOfBankerOutCard(cbHandCardData, cbHandCardCount, wMeChairID, OutCardResult) ;
	}
	//ѹ��
	else
	{	
		//��������
		if(wMeChairID==m_wBankerUser) BankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount, OutCardResult) ;
		//�����¼�
		else if(wMeChairID==wUndersideOfBanker) UndersideOfBankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount, OutCardResult) ;
		//�����ϼ�
		else if(wMeChairID==wUpsideOfBanker) UpsideOfBankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount, OutCardResult) ;
	}
	return true ;
}

//����ը��
void CAndroidAI::GetAllBomCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbBomCardData[], unsigned char &cbBomCardCount)
{
	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//��С����
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbBomCardCount = 0 ;

	if(cbHandCardCount<2) return ;

	//˫��ը��
	if(0x4F==cbTmpCardData[0] && 0x4E==cbTmpCardData[1])
	{
		cbBomCardData[cbBomCardCount++] = cbTmpCardData[0] ;
		cbBomCardData[cbBomCardCount++] = cbTmpCardData[1] ;
	}

	//�˿˷���
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//��������
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//����ͬ��
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//��ȡ�˿�
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//���ñ���
			cbSameCount++;
		}

		if(4==cbSameCount)
		{
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i] ;
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i+1] ;
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i+2] ;
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i+3] ;
		}

		//��������
		i+=cbSameCount-1;
	}
}

//����˳��
void CAndroidAI::GetAllLineCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbLineCardData[], unsigned char &cbLineCardCount)
{
	unsigned char cbTmpCard[MAX_COUNT] ;
	memcpy(cbTmpCard, cbHandCardData, cbHandCardCount) ;
	//��С����
	SortCardList(cbTmpCard, cbHandCardCount, ST_ORDER) ;

	cbLineCardCount = 0 ;

	//����У��
	if(cbHandCardCount<5) return ;

	unsigned char cbFirstCard = 0 ;
	//ȥ��2����
	for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

	unsigned char cbSingleLineCard[12] ;
	unsigned char cbSingleLineCount=0 ;
	unsigned char cbLeftCardCount = cbHandCardCount ;
	bool bFindSingleLine = true ;

	//�����ж�
	while (cbLeftCardCount>=5 && bFindSingleLine)
	{
		cbSingleLineCount=1 ;
		bFindSingleLine = false ;
		unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
		cbSingleLineCard[cbSingleLineCount-1] = cbTmpCard[cbFirstCard] ;
		for (unsigned char i=cbFirstCard+1; i<cbLeftCardCount; i++)
		{
			unsigned char cbCardData=cbTmpCard[i];

			//�����ж�
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
			//ͬ���ж�
			else if(GetCardValue(cbLastCard)!=GetCardValue(cbCardData))
			{
				cbLastCard = cbCardData ;
				cbSingleLineCard[cbSingleLineCount] = cbCardData ;
				++cbSingleLineCount ;
			}					
		}

		//��������
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

//��������
void CAndroidAI::GetAllThreeCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbThreeCardData[], unsigned char &cbThreeCardCount)
{
	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//��С����
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbThreeCardCount = 0 ;

	//�˿˷���
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//��������
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//����ͬ��
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//��ȡ�˿�
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//���ñ���
			cbSameCount++;
		}

		if(cbSameCount>=3)
		{
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i] ;
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i+1] ;
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i+2] ;	
		}

		//��������
		i+=cbSameCount-1;
	}
}

//��������
void CAndroidAI::GetAllDoubleCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbDoubleCardData[], unsigned char &cbDoubleCardCount)
{
	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//��С����
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbDoubleCardCount = 0 ;

	//�˿˷���
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//��������
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//����ͬ��
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//��ȡ�˿�
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//���ñ���
			cbSameCount++;
		}

		if(cbSameCount>=2)
		{
			cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i] ;
			cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i+1] ;
		}

		//��������
		i+=cbSameCount-1;
	}
}

//��������
void CAndroidAI::GetAllSingleCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbSingleCardData[], unsigned char &cbSingleCardCount)
{
	cbSingleCardCount =0 ;

	unsigned char cbTmpCardData[MAX_COUNT] ;
	memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount) ;

	//��С����
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	//�˿˷���
	for (unsigned char i=0;i<cbHandCardCount;i++)
	{
		//��������
		unsigned char cbSameCount=1;
		unsigned char cbLogicValue=GetCardLogicValue(cbTmpCardData[i]);

		//����ͬ��
		for (unsigned char j=i+1;j<cbHandCardCount;j++)
		{
			//��ȡ�˿�
			if (GetCardLogicValue(cbTmpCardData[j])!=cbLogicValue) break;

			//���ñ���
			cbSameCount++;
		}

		if(cbSameCount==1)
		{
			cbSingleCardData[cbSingleCardCount++] = cbTmpCardData[i] ;
		}

		//��������
		i+=cbSameCount-1;
	}
}

//��������
void CAndroidAI::AnalyseOutCardType(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12+1])
{
	my_memset(CardTypeResult, sizeof(CardTypeResult[0])*12) ;
	unsigned char cbTmpCardData[MAX_COUNT] ;
	//�����˿ˣ���ֹ����ʱ�ı��˿�
	unsigned char cbReserveCardData[MAX_COUNT] ;
	memcpy(cbReserveCardData, cbHandCardData, cbHandCardCount) ;	
	SortCardList(cbReserveCardData, cbHandCardCount, ST_ORDER) ;
	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	//��������
	for(unsigned char i=0; i<cbHandCardCount; ++i)
	{
		unsigned char Index = CardTypeResult[CT_SINGLE].cbCardTypeCount ;
		CardTypeResult[CT_SINGLE].cbCardType = CT_SINGLE ;
		CardTypeResult[CT_SINGLE].cbCardData[Index][0] = cbTmpCardData[i] ;
		CardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] = 1 ;
		CardTypeResult[CT_SINGLE].cbCardTypeCount++ ;	

		ASSERT(CardTypeResult[CT_SINGLE].cbCardTypeCount<MAX_TYPE_COUNT) ;
	}

	//��������
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

	//��������
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

	//ը������
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
	//��������
	{
		//�ָ��˿ˣ���ֹ����ʱ�ı��˿�
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		unsigned char cbFirstCard = 0 ;
		//ȥ��2����
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

		//�����ж�
		while (cbLeftCardCount>=5 && bFindSingleLine)
		{
			cbSingleLineCount=1 ;
			bFindSingleLine = false ;
			unsigned char cbLastCard = cbTmpCardData[cbFirstCard] ;
			cbSingleLineCard[cbSingleLineCount-1] = cbTmpCardData[cbFirstCard] ;
			for (unsigned char i=cbFirstCard+1; i<cbLeftCardCount; i++)
			{
				unsigned char cbCardData=cbTmpCardData[i];

				//�����ж�
				if (1!=(GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbCardData)) && GetCardValue(cbLastCard)!=GetCardValue(cbCardData)) 
				{
					cbLastCard = cbTmpCardData[i] ;
					//�Ƿ�Ϸ�
					if(cbSingleLineCount<5) 
					{
						cbSingleLineCount = 1 ;
						cbSingleLineCard[cbSingleLineCount-1] = cbTmpCardData[i] ;
						continue ;
					}
					else break ;
				}
				//ͬ���ж�
				else if(GetCardValue(cbLastCard)!=GetCardValue(cbCardData))
				{
					cbLastCard = cbCardData ;
					cbSingleLineCard[cbSingleLineCount] = cbCardData ;
					++cbSingleLineCount ;
				}
			}

			//��������
			if(cbSingleLineCount>=5)
			{
				unsigned char Index ;
				//��������
				unsigned char cbStart=0 ;
				//�Ӵ�С
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
				//��С����
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

	//��������
	{
		//�ָ��˿ˣ���ֹ����ʱ�ı��˿�
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		//�����ж�
		unsigned char cbFirstCard = 0 ;
		//ȥ��2����
		for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCardData[i])<15)	{cbFirstCard = i ; break ;}

		unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
		bool bFindDoubleLine = true ;
		unsigned char cbDoubleLineCount = 0 ;
		unsigned char cbDoubleLineCard[24] ;
		//��ʼ�ж�
		while (cbLeftCardCount>=6 && bFindDoubleLine)
		{
			unsigned char cbLastCard = cbTmpCardData[cbFirstCard] ;
			unsigned char cbSameCount = 1 ;
			cbDoubleLineCount = 0 ;
			bFindDoubleLine=false ;
			for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
			{
				//����ͬ��
				while (GetCardLogicValue(cbLastCard)==GetCardLogicValue(cbTmpCardData[i]) && i<cbLeftCardCount+cbFirstCard)
				{
					++cbSameCount;
					++i ;
				}

				unsigned char cbLastDoubleCardValue ;
				if(cbDoubleLineCount>0) cbLastDoubleCardValue = GetCardLogicValue(cbDoubleLineCard[cbDoubleLineCount-1]) ;
				//���¿�ʼ
				if((cbSameCount<2 || (cbDoubleLineCount>0 && (cbLastDoubleCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
				{
					if(cbDoubleLineCount>=6) break ;
					//����
					if(cbSameCount>=2) i-=cbSameCount ;
					cbLastCard = cbTmpCardData[i] ;
					cbDoubleLineCount = 0 ;
				}
				//��������
				else if(cbSameCount>=2)
				{
					cbDoubleLineCard[cbDoubleLineCount] = cbTmpCardData[i-cbSameCount] ;
					cbDoubleLineCard[cbDoubleLineCount+1] = cbTmpCardData[i-cbSameCount+1] ;
					cbDoubleLineCount += 2 ;

					//��β�ж�
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

			//��������
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

	//��������
	{
		//�ָ��˿ˣ���ֹ����ʱ�ı��˿�
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		//�����ж�
		unsigned char cbFirstCard = 0 ;
		//ȥ��2����
		for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCardData[i])<15)	{cbFirstCard = i ; break ;}

		unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
		bool bFindThreeLine = true ;
		unsigned char cbThreeLineCount = 0 ;
		unsigned char cbThreeLineCard[20] ;
		//��ʼ�ж�
		while (cbLeftCardCount>=6 && bFindThreeLine)
		{
			unsigned char cbLastCard = cbTmpCardData[cbFirstCard] ;
			unsigned char cbSameCount = 1 ;
			cbThreeLineCount = 0 ;
			bFindThreeLine = false ;
			for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
			{
				//����ͬ��
				while (GetCardLogicValue(cbLastCard)==GetCardLogicValue(cbTmpCardData[i]) && i<cbLeftCardCount+cbFirstCard)
				{
					++cbSameCount;
					++i ;
				}

				unsigned char cbLastThreeCardValue ;
				if(cbThreeLineCount>0) cbLastThreeCardValue = GetCardLogicValue(cbThreeLineCard[cbThreeLineCount-1]) ;

				//���¿�ʼ
				if((cbSameCount<3 || (cbThreeLineCount>0&&(cbLastThreeCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
				{
					if(cbThreeLineCount>=6) break ;

					if(cbSameCount>=3) i-=cbSameCount ;
					cbLastCard = cbTmpCardData[i] ;
					cbThreeLineCount = 0 ;
				}
				//��������
				else if(cbSameCount>=3)
				{
					cbThreeLineCard[cbThreeLineCount] = cbTmpCardData[i-cbSameCount] ;
					cbThreeLineCard[cbThreeLineCount+1] = cbTmpCardData[i-cbSameCount+1] ;
					cbThreeLineCard[cbThreeLineCount+2] = cbTmpCardData[i-cbSameCount+2] ;
					cbThreeLineCount += 3 ;

					//��β�ж�
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

			//��������
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
	//����һ��
	{
		//�ָ��˿ˣ���ֹ����ʱ�ı��˿�
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		unsigned char cbHandThreeCard[MAX_COUNT];
		unsigned char cbHandThreeCount=0 ;

		//�Ƴ�ը��
		unsigned char cbAllBomCardData[MAX_COUNT] ;
		unsigned char cbAllBomCardCount=0 ;
		GetAllBomCard(cbTmpCardData, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount) ;
		RemoveCard(cbAllBomCardData, cbAllBomCardCount, cbTmpCardData, cbHandCardCount) ;

		GetAllThreeCard(cbTmpCardData, cbHandCardCount-cbAllBomCardCount, cbHandThreeCard, cbHandThreeCount) ;

		{
			unsigned char Index ;
			//ȥ������
			unsigned char cbRemainCardData[MAX_COUNT] ;
			memcpy(cbRemainCardData, cbTmpCardData, cbHandCardCount-cbAllBomCardCount) ;
			unsigned char cbRemainCardCount=cbHandCardCount-cbAllBomCardCount-cbHandThreeCount ;
			RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCardData, cbHandCardCount-cbAllBomCardCount) ;
			//������һ��
			for(unsigned char i=0; i<cbHandThreeCount; i+=3)
			{
				//������һ��
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

		//��������
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
				//�����ж�
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


				//�Ƴ���������Ӧ���Ƴ�ը�����ȣ�
				memcpy(cbRemainCard, cbTmpCardData, (cbHandCardCount-cbAllBomCardCount)*sizeof(unsigned char));
				RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCard, cbHandCardCount-cbAllBomCardCount) ;

				for(unsigned char start=0; start<cbThreeLineCardCount-3; start+=3)
				{
					//��˳��Ŀ
					unsigned char cbThisTreeLineCardCount = cbThreeLineCardCount-start ;
					//���Ƹ���
					unsigned char cbSingleCardCount=(cbThisTreeLineCardCount)/3;

					//���Ʋ���
					if(cbRemainCardCount<cbSingleCardCount) continue ;

					//�������
					unsigned char cbComCard[5];
					unsigned char cbComResCard[254][5] ;
					unsigned char cbComResLen=0 ;

					Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, cbSingleCardCount, cbRemainCardCount, cbSingleCardCount);
					for(unsigned char i=0; i<cbComResLen; ++i)
					{
						Index = CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount ;
						CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardType = CT_THREE_LINE_TAKE_ONE ;
						//��������
						memcpy(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index], cbThreeLineCard+start, sizeof(unsigned char)*cbThisTreeLineCardCount);
						//���浥��
						memcpy(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index]+cbThisTreeLineCardCount, cbComResCard[i], cbSingleCardCount) ;


						CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbEachHandCardCount[Index] = cbThisTreeLineCardCount+cbSingleCardCount ;
						CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount++ ;

						ASSERT(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount<MAX_TYPE_COUNT) ;
					}

				}

				//�Ƴ�����
				bFindThreeLine = true ;
				RemoveCard(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount) ;
				cbLeftThreeCardCount -= cbThreeLineCardCount ;
			}
		}
	}

	//����һ��
	{
		//�ָ��˿ˣ���ֹ����ʱ�ı��˿�
		memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

		unsigned char cbHandThreeCard[MAX_COUNT];
		unsigned char cbHandThreeCount=0 ;
		unsigned char cbRemainCarData[MAX_COUNT] ;
		unsigned char cbRemainCardCount=0 ;

		//��ȡ����
		GetAllThreeCard(cbTmpCardData, cbHandCardCount, cbHandThreeCard, cbHandThreeCount) ;

		//�Ƴ���������Ӧ���Ƴ�ը�����ȣ�
		memcpy(cbRemainCarData, cbTmpCardData, cbHandCardCount) ;
		RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCarData, cbHandCardCount) ;
		cbRemainCardCount = cbHandCardCount-cbHandThreeCount ;

		//��ȡ����
		unsigned char cbAllDoubleCardData[MAX_COUNT] ;
		unsigned char cbAllDoubleCardCount=0 ;
		GetAllDoubleCard(cbRemainCarData, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;

		//������һ��
		for(unsigned char i=0; i<cbHandThreeCount; i+=3)
		{
			unsigned char Index ;

			//������һ��
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

		//��������
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
				//�����ж�
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
					//��˳��Ŀ
					unsigned char cbThisTreeLineCardCount = cbThreeLineCardCount-start ;
					//��������
					unsigned char cbDoubleCardCount=((cbThisTreeLineCardCount)/3);

					//���Ʋ���
					if(cbRemainCardCount<cbDoubleCardCount) continue ;

					unsigned char cbDoubleCardIndex[10]; //�����±�
					for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
						cbDoubleCardIndex[j]=i ;

					//�������
					unsigned char cbComCard[5];
					unsigned char cbComResCard[254][5] ;
					unsigned char cbComResLen=0 ;

					//���ö��Ƶ��±�����ϣ��ٸ����±���ȡ������
					Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, cbDoubleCardCount, cbAllDoubleCardCount/2, cbDoubleCardCount);

					ASSERT(cbComResLen<=254) ;

					for(unsigned char i=0; i<cbComResLen; ++i)
					{
						Index = CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount ;
						CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardType = CT_THREE_LINE_TAKE_TWO ;
						//��������
						memcpy(CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index], cbThreeLineCard+start, sizeof(unsigned char)*cbThisTreeLineCardCount);
						//�������
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
				//�Ƴ�����
				bFindThreeLine = true ;				
				RemoveCard(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount) ;
				cbLeftThreeCardCount -= cbThreeLineCardCount ;
			}
		}
	}
	//�Ĵ�����
	/*
	{
	//�ָ��˿ˣ���ֹ����ʱ�ı��˿�
	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	unsigned char cbFirstCard = 0 ;
	//ȥ������
	for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

	unsigned char cbHandAllFourCardData[MAX_COUNT] ;
	unsigned char cbHandAllFourCardCount=0;
	//��ȡ����
	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

	//�Ƴ�����
	unsigned char cbRemainCard[MAX_COUNT];
	unsigned char cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
	memcpy(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(unsigned char));
	RemoveCard(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

	for(unsigned char Start=0; Start<cbHandAllFourCardCount; Start += 4)
	{
	unsigned char Index ;
	//�������
	unsigned char cbComCard[5];
	unsigned char cbComResCard[254][5] ;
	unsigned char cbComResLen=0 ;
	//�������
	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, 2, cbRemainCardCount, 2);
	for(unsigned char i=0; i<cbComResLen; ++i)
	{
	//���ܴ���
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


	//�Ĵ�����
	/*
	{
	//�ָ��˿ˣ���ֹ����ʱ�ı��˿�
	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	unsigned char cbFirstCard = 0 ;
	//ȥ������
	for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

	unsigned char cbHandAllFourCardData[MAX_COUNT] ;
	unsigned char cbHandAllFourCardCount=0;

	//��ȡ����
	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

	//�Ƴ�����
	unsigned char cbRemainCard[MAX_COUNT];
	unsigned char cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
	memcpy(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(unsigned char));
	RemoveCard(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

	for(unsigned char Start=0; Start<cbHandAllFourCardCount; Start += 4)
	{
	//��ȡ����
	unsigned char cbAllDoubleCardData[MAX_COUNT] ;
	unsigned char cbAllDoubleCardCount=0 ;
	GetAllDoubleCard(cbRemainCard, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;

	unsigned char cbDoubleCardIndex[10]; //�����±�
	for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
	cbDoubleCardIndex[j]=i ;

	//�������
	unsigned char cbComCard[5];
	unsigned char cbComResCard[255][5] ;
	unsigned char cbComResLen=0 ;

	//���ö��Ƶ��±�����ϣ��ٸ����±���ȡ������
	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, 2, cbAllDoubleCardCount/2, 2);
	for(unsigned char i=0; i<cbComResLen; ++i)
	{
	unsigned char Index = CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount ;
	CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardType = CT_FOUR_LINE_TAKE_TWO ;
	memcpy(CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index], cbHandAllFourCardData+Start, 4) ;

	//�������
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

//��������
void CAndroidAI::AnalyseOutCardType(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char const cbTurnCardData[], unsigned char const cbTurnCardCount,tagOutCardTypeResult CardTypeResult[12+1])
{
	my_memset(CardTypeResult, sizeof(CardTypeResult[0])*12) ;
	//����У��
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
		//˫��ը��
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
		//ը������
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
	case CT_SINGLE:				//��������
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
	case CT_DOUBLE:				//��������
		{
			//�˿˷���
			for (unsigned char i=0;i<cbHandCardCount;i++)
			{
				//��������
				unsigned char cbSameCount=1;
				unsigned char cbLogicValue=GetCardLogicValue(cbTmpCard[i]);

				//����ͬ��
				for (unsigned char j=i+1;j<cbHandCardCount;j++)
				{
					//��ȡ�˿�
					if (GetCardLogicValue(cbTmpCard[j])!=cbLogicValue) break;

					//���ñ���
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
				//��������
				i+=cbSameCount-1;
			}
			break ;
		}
	case CT_THREE:				//��������
		{
			//�˿˷���
			for (unsigned char i=0;i<cbHandCardCount;i++)
			{
				//��������
				unsigned char cbSameCount=1;
				unsigned char cbLogicValue=GetCardLogicValue(cbTmpCard[i]);

				//����ͬ��
				for (unsigned char j=i+1;j<cbHandCardCount;j++)
				{
					//��ȡ�˿�
					if (GetCardLogicValue(cbTmpCard[j])!=cbLogicValue) break;

					//���ñ���
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
				//��������
				i+=cbSameCount-1;
			}
			break ;
		}
	case CT_SINGLE_LINE:		//��������
		{
			unsigned char cbFirstCard = 0 ;
			//ȥ��2����
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

			unsigned char cbSingleLineCard[12] ;
			unsigned char cbSingleLineCount=1 ;
			unsigned char cbLeftCardCount = cbHandCardCount ;
			bool bFindSingleLine = true ;

			//�����ж�
			while (cbLeftCardCount>=cbTurnCardCount && bFindSingleLine)
			{
				cbSingleLineCount=1 ;
				bFindSingleLine = false ;
				unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
				cbSingleLineCard[cbSingleLineCount-1] = cbTmpCard[cbFirstCard] ;
				for (unsigned char i=cbFirstCard+1; i<cbLeftCardCount; i++)
				{
					unsigned char cbCardData=cbTmpCard[i];

					//�����ж�
					if (1!=(GetCardLogicValue(cbLastCard)-GetCardLogicValue(cbCardData)) && GetCardValue(cbLastCard)!=GetCardValue(cbCardData)) 
					{
						cbLastCard = cbTmpCard[i] ;
						//�Ƿ�Ϸ�
						if(cbSingleLineCount<cbTurnCardCount) 
						{
							cbSingleLineCount = 1 ;
							cbSingleLineCard[cbSingleLineCount-1] = cbTmpCard[i] ;
							continue ;
						}
						else break ;
					}
					//ͬ���ж�
					else if(GetCardValue(cbLastCard)!=GetCardValue(cbCardData))
					{
						cbLastCard = cbCardData ;
						cbSingleLineCard[cbSingleLineCount] = cbCardData ;
						++cbSingleLineCount ;
					}
				}

				//��������
				if(cbSingleLineCount>=cbTurnCardCount && GetCardLogicValue(cbSingleLineCard[0])>GetCardLogicValue(cbTurnCardData[0]))
				{
					unsigned char Index ;
					unsigned char cbStart=0 ;
					//��������
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
	case CT_DOUBLE_LINE:		//��������
		{
			//�����ж�
			unsigned char cbFirstCard = 0 ;
			//ȥ��2����
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

			unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
			bool bFindDoubleLine = true ;
			unsigned char cbDoubleLineCount = 0 ;
			unsigned char cbDoubleLineCard[24] ;
			//��ʼ�ж�
			while (cbLeftCardCount>=cbTurnCardCount && bFindDoubleLine)
			{
				unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
				unsigned char cbSameCount = 1 ;
				cbDoubleLineCount = 0 ;
				bFindDoubleLine=false ;
				for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
				{
					//����ͬ��
					while (GetCardValue(cbLastCard)==GetCardValue(cbTmpCard[i]) && i<cbLeftCardCount+cbFirstCard)
					{
						++cbSameCount;
						++i ;
					}

					unsigned char cbLastDoubleCardValue ;
					if(cbDoubleLineCount>0) cbLastDoubleCardValue = GetCardLogicValue(cbDoubleLineCard[cbDoubleLineCount-1]) ;
					//���¿�ʼ
					if((cbSameCount<2 || (cbDoubleLineCount>0 && (cbLastDoubleCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
					{
						if(cbDoubleLineCount>=cbTurnCardCount) break ;

						if(cbSameCount>=2) i-=cbSameCount ;

						cbLastCard = cbTmpCard[i] ;
						cbDoubleLineCount = 0 ;
					}
					//��������
					else if(cbSameCount>=2)
					{
						cbDoubleLineCard[cbDoubleLineCount] = cbTmpCard[i-cbSameCount] ;
						cbDoubleLineCard[cbDoubleLineCount+1] = cbTmpCard[i-cbSameCount+1] ;
						cbDoubleLineCount += 2 ;

						//��β�ж�
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

				//��������
				if(cbDoubleLineCount>=cbTurnCardCount)
				{
					unsigned char Index ;
					unsigned char cbStart=0 ;
					//��������
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
	case CT_THREE_LINE:			//��������
		{
			//�����ж�
			unsigned char cbFirstCard = 0 ;
			//ȥ��2����
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardLogicValue(cbTmpCard[i])<15)	{cbFirstCard = i ; break ;}

			unsigned char cbLeftCardCount = cbHandCardCount-cbFirstCard ;
			bool bFindThreeLine = true ;
			unsigned char cbThreeLineCount = 0 ;
			unsigned char cbThreeLineCard[20] ;
			//��ʼ�ж�
			while (cbLeftCardCount>=cbTurnCardCount && bFindThreeLine)
			{
				unsigned char cbLastCard = cbTmpCard[cbFirstCard] ;
				unsigned char cbSameCount = 1 ;
				cbThreeLineCount = 0 ;
				bFindThreeLine = false ;
				for(unsigned char i=cbFirstCard+1 ; i<cbLeftCardCount+cbFirstCard ; ++i)
				{
					//����ͬ��
					while (GetCardValue(cbLastCard)==GetCardValue(cbTmpCard[i]) && i<cbLeftCardCount+cbFirstCard)
					{
						++cbSameCount;
						++i ;
					}

					unsigned char cbLastThreeCardValue ;
					if(cbThreeLineCount>0) cbLastThreeCardValue = GetCardLogicValue(cbThreeLineCard[cbThreeLineCount-1]) ;

					//���¿�ʼ
					if((cbSameCount<3 || (cbThreeLineCount>0&&(cbLastThreeCardValue-GetCardLogicValue(cbLastCard))!=1)) && i<=cbLeftCardCount+cbFirstCard)
					{
						if(cbThreeLineCount>=cbTurnCardCount) break ;

						if(cbSameCount>=3) i-= 3 ;
						cbLastCard = cbTmpCard[i] ;
						cbThreeLineCount = 0 ;
					}
					//��������
					else if(cbSameCount>=3)
					{
						cbThreeLineCard[cbThreeLineCount] = cbTmpCard[i-cbSameCount] ;
						cbThreeLineCard[cbThreeLineCount+1] = cbTmpCard[i-cbSameCount+1] ;
						cbThreeLineCard[cbThreeLineCount+2] = cbTmpCard[i-cbSameCount+2] ;
						cbThreeLineCount += 3 ;

						//��β�ж�
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

				//��������
				if(cbThreeLineCount>=cbTurnCardCount)
				{
					unsigned char Index ;
					unsigned char cbStart=0 ;
					//��������
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
	case CT_THREE_LINE_TAKE_ONE://����һ��
		{
			unsigned char cbTurnThreeCard[MAX_COUNT]; 
			unsigned char cbTurnThreeCount=0;
			unsigned char cbHandThreeCard[MAX_COUNT];
			unsigned char cbHandThreeCount=0 ;
			unsigned char cbSingleCardCount=cbTurnCardCount/4;

			//�Ƴ�ը��
			unsigned char cbAllBomCardData[MAX_COUNT] ;
			unsigned char cbAllBomCardCount=0 ;
			GetAllBomCard(cbTmpCard, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount) ;
			RemoveCard(cbAllBomCardData, cbAllBomCardCount, cbTmpCard, cbHandCardCount) ;

			//�����˿�
			GetAllThreeCard(cbTurnCardData, cbTurnCardCount, cbTurnThreeCard, cbTurnThreeCount) ;

			unsigned char cbFirstCard = 0 ;

			//ȥ��2����
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
							//�����ж�
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

							//�������
							unsigned char cbComCard[5];
							unsigned char cbComResCard[254][5] ;
							unsigned char cbComResLen=0 ;
							Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, cbSingleCardCount, (cbHandCardCount-cbAllBomCardCount)-cbTurnThreeCount, cbSingleCardCount);
							for(unsigned char i=0; i<cbComResLen; ++i)
							{
								Index = CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardTypeCount ;
								CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardType = CT_THREE_LINE_TAKE_ONE;
								//��������
								memcpy(CardTypeResult[CT_THREE_LINE_TAKE_ONE].cbCardData[Index], cbThreeLineCard, sizeof(unsigned char)*cbTurnThreeCount);
								//���浥��
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
	case CT_THREE_LINE_TAKE_TWO://����һ��
		{
			unsigned char cbTurnThreeCard[MAX_COUNT]; 
			unsigned char cbTurnThreeCount=0;
			unsigned char cbHandThreeCard[MAX_COUNT];
			unsigned char cbHandThreeCount=0 ;
			unsigned char cbDoubleCardCount=cbTurnCardCount/5;

			//�����˿�
			GetAllThreeCard(cbTurnCardData, cbTurnCardCount, cbTurnThreeCard, cbTurnThreeCount) ;

			unsigned char cbFirstCard = 0 ;

			//ȥ��2����
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
						//�����ж�
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


						unsigned char cbDoubleCardIndex[10]; //�����±�
						for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
							cbDoubleCardIndex[j]=i ;

						//�������
						unsigned char cbComCard[5];
						unsigned char cbComResCard[254][5] ;
						unsigned char cbComResLen=0 ;

						//���ö��Ƶ��±�����ϣ��ٸ����±���ȡ������
						Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, cbDoubleCardCount, cbAllDoubleCardCount/2, cbDoubleCardCount);
						for(unsigned char i=0; i<cbComResLen; ++i)
						{
							Index = CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardTypeCount ;
							CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardType = CT_THREE_LINE_TAKE_TWO ;
							//��������
							memcpy(CardTypeResult[CT_THREE_LINE_TAKE_TWO].cbCardData[Index], cbThreeLineCard, sizeof(unsigned char)*cbTurnThreeCount);
							//�������
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
	case CT_FOUR_LINE_TAKE_ONE://�Ĵ�����
		{
			unsigned char cbFirstCard = 0 ;
			//ȥ������
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCard[i])!=0x40)	{cbFirstCard = i ; break ;}

			unsigned char cbHandAllFourCardData[MAX_COUNT] ;
			unsigned char cbHandAllFourCardCount=0;
			unsigned char cbTurnAllFourCardData[MAX_COUNT];
			unsigned char cbTurnAllFourCardCount=0;
			//��ȡ����
			GetAllBomCard(cbTmpCard+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;
			GetAllBomCard(cbTurnCardData, cbTurnCardCount, cbTurnAllFourCardData, cbTurnAllFourCardCount) ;

			if(cbHandAllFourCardCount>0 && GetCardLogicValue(cbHandAllFourCardData[0])<GetCardLogicValue(cbTurnAllFourCardData[0])) return ;


			unsigned char cbCanOutFourCardData[MAX_COUNT] ;
			unsigned char cbCanOutFourCardCount=0 ;
			//�ɳ�����
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
				//�������
				unsigned char cbComCard[5];
				unsigned char cbComResCard[254][5] ;
				unsigned char cbComResLen=0 ;
				//�������
				Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, 2, cbHandCardCount-cbCanOutFourCardCount, 2);
				for(unsigned char i=0; i<cbComResLen; ++i)
				{
					//���ܴ���
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
	case CT_FOUR_LINE_TAKE_TWO://�Ĵ�����
		{
			unsigned char cbFirstCard = 0 ;
			//ȥ������
			for(unsigned char i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCard[i])!=0x40)	{cbFirstCard = i ; break ;}

			unsigned char cbHandAllFourCardData[MAX_COUNT] ;
			unsigned char cbHandAllFourCardCount=0;
			unsigned char cbTurnAllFourCardData[MAX_COUNT];
			unsigned char cbTurnAllFourCardCount=0;
			//��ȡ����
			GetAllBomCard(cbTmpCard+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;
			GetAllBomCard(cbTurnCardData, cbTurnCardCount, cbTurnAllFourCardData, cbTurnAllFourCardCount) ;

			if(cbHandAllFourCardCount>0 && GetCardLogicValue(cbHandAllFourCardData[0])<GetCardLogicValue(cbTurnAllFourCardData[0])) return ;


			unsigned char cbCanOutFourCardData[MAX_COUNT] ;
			unsigned char cbCanOutFourCardCount=0 ;
			//�ɳ�����
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


				unsigned char cbDoubleCardIndex[10]; //�����±�
				for(unsigned char i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
					cbDoubleCardIndex[j]=i ;

				//�������
				unsigned char cbComCard[5];
				unsigned char cbComResCard[254][5] ;
				unsigned char cbComResLen=0 ;

				//���ö��Ƶ��±�����ϣ��ٸ����±���ȡ������
				Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, 2, cbAllDoubleCardCount/2, 2);
				for(unsigned char i=0; i<cbComResLen; ++i)
				{
					unsigned char Index = CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardTypeCount ;
					CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardType = CT_FOUR_LINE_TAKE_TWO ;
					memcpy(CardTypeResult[CT_FOUR_LINE_TAKE_TWO].cbCardData[Index], cbCanOutFourCardData+Start, 4) ;

					//�������
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
	case CT_BOMB_CARD:			//ը������
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
	case CT_MISSILE_CARD:		//�������
		{
			//û�бȻ�����������
			break;
		}
	default:
		ASSERT(false) ;
		break;
	}

}

/********************************************************************
��������Combination
������
cbCombineCardData���洢��������Ͻ��
cbResComLen���ѵõ�����ϳ��ȣ���ʼ����ʱ�˲���Ϊ0
cbResultCardData���洢���е���Ͻ��
cbResCardLen��cbResultCardData�ĵ�һ�±�ĳ��ȣ���Ͻ���ĸ���
cbSrcCardData����Ҫ����ϵ�����
cbSrcLen��cbSrcCardData��������Ŀ
cbCombineLen2��cbCombineLen1����ϵĳ��ȣ���ʼ����ʱ������ȡ�
*********************************************************************/
//����㷨
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

//�����㷨
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

//���Ƹ���
unsigned char CAndroidAI::AnalyseSinleCardCount(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char const cbWantOutCardData[], unsigned char const cbWantOutCardCount, unsigned char cbSingleCardData[])
{
	unsigned char cbRemainCard[MAX_COUNT] ;
	unsigned char cbRemainCardCount=0 ;
	memcpy(cbRemainCard, cbHandCardData, cbHandCardCount) ;
	SortCardList(cbRemainCard, cbHandCardCount, ST_ORDER) ;

	if(cbWantOutCardCount!=0) RemoveCard(cbWantOutCardData, cbWantOutCardCount, cbRemainCard, cbHandCardCount) ;
	cbRemainCardCount = cbHandCardCount-cbWantOutCardCount ;

	//����ָ��
	typedef void (CAndroidAI::*pGetAllCardFun)(unsigned char const [], unsigned char const , unsigned char[], unsigned char &); 

	//ָ������
	pGetAllCardFun GetAllCardFunArray[4] ;
	GetAllCardFunArray[0] = &CAndroidAI::GetAllBomCard;		//ը������
	GetAllCardFunArray[1] = &CAndroidAI::GetAllLineCard;	//˳�Ӻ���
	GetAllCardFunArray[2] = &CAndroidAI::GetAllThreeCard;	//��������
	GetAllCardFunArray[3] = &CAndroidAI::GetAllDoubleCard;	//���Ӻ���

	//ָ�������±�
	unsigned char cbIndexArray[4] = {0,1,2,3} ;
	//���н��
	unsigned char cbPermutationRes[24][4] ;
	unsigned char cbLen=0 ;
	//��������
	Permutation(cbIndexArray, 0, 4, cbPermutationRes, cbLen) ;
	ASSERT(cbLen==24) ;
	if(cbLen!=24) return MAX_COUNT ;

	//������Ŀ
	unsigned char cbMinSingleCardCount = MAX_COUNT ;
	//������Сֵ
	for(unsigned char i=0; i<24; ++i)
	{
		//��������
		unsigned char cbTmpCardData[MAX_COUNT] ;
		unsigned char cbTmpCardCount = cbRemainCardCount ;
		memcpy(cbTmpCardData, cbRemainCard, cbRemainCardCount) ;

		for(unsigned char j=0; j<4; ++j)
		{
			unsigned char Index = cbPermutationRes[i][j] ;

			//У���±�
			ASSERT(Index>=0 && Index<4) ;
			if(Index<0 || Index>=4) return MAX_COUNT ;

			pGetAllCardFun pTmpGetAllCardFun = GetAllCardFunArray[Index] ;

			//��ȡ�˿�
			unsigned char cbGetCardData[MAX_COUNT] ;
			unsigned char cbGetCardCount=0 ;
			//��Ա����
			((*this).*pTmpGetAllCardFun)(cbTmpCardData, cbTmpCardCount, cbGetCardData, cbGetCardCount) ;

			//ɾ���˿�
			if(cbGetCardCount!=0) RemoveCard(cbGetCardData, cbGetCardCount, cbTmpCardData, cbTmpCardCount) ;
			cbTmpCardCount -= cbGetCardCount ;
		}

		//���㵥��
		unsigned char cbSingleCard[MAX_COUNT] ;
		unsigned char cbSingleCardCount=0 ;
		GetAllSingleCard(cbTmpCardData, cbTmpCardCount, cbSingleCard, cbSingleCardCount) ;
		cbMinSingleCardCount = cbMinSingleCardCount > cbSingleCardCount ? cbSingleCardCount : cbMinSingleCardCount ;
	}

	return cbMinSingleCardCount ;
}


//�������ƣ��ȳ��ƣ�
void CAndroidAI::BankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, tagOutCardResult & OutCardResult) 
{
	//���±�û��
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//��ʼ����
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

	int wUndersideOfBanker = (m_wBankerUser+1)%GAME_PLAYER ;	//�����¼�
	int wUpsideOfBanker = (wUndersideOfBanker+1)%GAME_PLAYER ;	//�����ϼ�

	//���ֻʣ˳�ƺ͵�ֻ�����ȳ�˳
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
		//˫��ը����һ��
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

	//������һֻ
	if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//����
	else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//ֻʣһ����
	else if(CT_ERROR!=GetCardType(cbHandCardData, cbHandCardCount))
	{
		OutCardResult.cbCardCount = cbHandCardCount ;
		memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount) ;
		return ;
	}

	//ֻʣһ�ź�һ��
	if(cbHandCardCount>=2)
	{
		//�ϼ��˿�
		tagOutCardTypeResult UpsideCanOutCardType1[13] ;
		my_memset(UpsideCanOutCardType1, sizeof(UpsideCanOutCardType1)) ;
		tagOutCardTypeResult UpsideCanOutCardType2[13] ;
		my_memset(UpsideCanOutCardType2, sizeof(UpsideCanOutCardType2)) ;

		//�¼��˿�
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
			//�����˿�
			tagOutCardTypeResult MeOutCardTypeResult[13] ;
			my_memset(MeOutCardTypeResult, sizeof(MeOutCardTypeResult)) ;
			AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;

			//���㵥��
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

			//��ը�������
			for(unsigned char cbCardType=CT_DOUBLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
			{

				tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType] ;

				//��ͬ���ͣ���ͬ���ȣ���������������ͬ���Ϳ��ܳ��Ȳ�һ��
				unsigned char cbThisHandCardCount = MAX_COUNT ;

				//�ϼ��˿�
				tagOutCardTypeResult UpsideOutCardTypeResult[13] ;
				my_memset(UpsideOutCardTypeResult, sizeof(UpsideOutCardTypeResult)) ;

				//�¼��˿�
				tagOutCardTypeResult UndersideOutCardTypeResult[13] ;
				my_memset(UndersideOutCardTypeResult, sizeof(UndersideOutCardTypeResult)) ;


				for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
				{
					unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

					//���·���
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

					//�з�����ѹס��
					if(UpsideOutCardTypeResult[cbCardType].cbCardTypeCount>0 || UndersideOutCardTypeResult[cbCardType].cbCardTypeCount>0)
					{
						continue ;
					}
					//�Ƿ��д��� 
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

					//����cbMinSingleCardCount[4]�����ֵ
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

					//������Сֵ
					if(cbMinSingleCountInFour>=cbTmpCount)
					{
						//��С����
						cbMinCardType = cbCardType ;
						//��С�����е���С����
						cbMinSingleCountInFour=cbTmpCount ;						
						//��С�����е���С��
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

				//ѡ����С��
				for(unsigned char i=0; i<4; ++i)
				{
					if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
						GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
						Index = cbIndex[i] ;
				}

				//������һֻ
				if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
				{
					OutCardResult.cbCardCount = 2 ;
					OutCardResult.cbResultCard[0] = 0x4f ;
					OutCardResult.cbResultCard[1] = 0x4e ;
					return ;
				}
				//����
				else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
				{
					OutCardResult.cbCardCount = 2 ;
					OutCardResult.cbResultCard[0] = 0x4f ;
					OutCardResult.cbResultCard[1] = 0x4e ;
					return ;
				}
				else
				{
					//���ñ���
					OutCardResult.cbCardCount=MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
					memcpy(OutCardResult.cbResultCard,MeOutCardTypeResult[cbMinCardType].cbCardData[Index],MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
					return ;
				}

				ASSERT(OutCardResult.cbCardCount>0) ;

				return ;
			}

			//��������˿�����5����û���ҵ��ʺϵ�����Ӵ����С
			if(OutCardResult.cbCardCount<=0 && (m_cbUserCardCount[wUndersideOfBanker]>=4 || m_cbUserCardCount[wUpsideOfBanker]>=4))
			{
				//ֻ��һ����ʱ���ܷŵ�����
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
	//��������2
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

		//���㵥��
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

		//��ը�������
		for(unsigned char cbCardType=CT_SINGLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType] ;
			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//����cbMinSingleCardCount[4]�����ֵ
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

				//������Сֵ
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//��С����
					cbMinCardType = cbCardType ;
					//��С�����е���С����
					cbMinSingleCountInFour=cbTmpCount ;						
					//��С�����е���С��
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//ѡ����С��
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//������һֻ
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//����
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//���ñ���
				OutCardResult.cbCardCount=CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[cbMinCardType].cbCardData[Index],CardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}
		//���ֻʣը��
		else
		{
			unsigned char Index=0 ;
			unsigned char cbLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			//��Сը��
			for(unsigned char i=0; i<CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
				if(cbLogicCardValue>GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
				{
					cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]) ;
					Index = i ;
				}

				//���ñ���
				OutCardResult.cbCardCount=CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[CT_BOMB_CARD].cbCardData[Index],CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index]*sizeof(unsigned char));

				return ;
		}

		//�����û���������ͳ���С��һ��
		OutCardResult.cbCardCount = 1 ;
		OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount-1] ;

		return ;
}

//�������ƣ�����ƣ�
void CAndroidAI::BankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult) 
{
	//��ʼ����
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//���±�û��
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//��������
	unsigned char cbOutCardType = GetCardType(cbTurnCardData,cbTurnCardCount) ;
	AnalyseOutCardType(cbHandCardData,cbHandCardCount,cbTurnCardData,cbTurnCardCount, CardTypeResult) ;

	int wUndersideUser = (m_wBankerUser+1)%GAME_PLAYER ;
	int wUpsideUser = (wUndersideUser+1)%GAME_PLAYER ;

	//ֻʣը��
	if(cbHandCardCount==CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
	{
		OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0] ;
		memcpy(OutCardResult.cbResultCard,  CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount) ;

		return ;
	}
	//˫��ը����һ��
	else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return  ;
	}

	//ȡ���ĸ���С����
	unsigned char cbMinSingleCardCount[4] ;
	cbMinSingleCardCount[0]=MAX_COUNT ;
	cbMinSingleCardCount[1]=MAX_COUNT ;
	cbMinSingleCardCount[2]=MAX_COUNT ;
	cbMinSingleCardCount[3]=MAX_COUNT ;
	unsigned char cbIndex[4]={0} ;	
	unsigned char cbMinSingleCountInFour=MAX_COUNT ;

	//�ɳ��˿ˣ������Ѿ����˵�ը���ˣ�
	tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType] ;

	for(unsigned char i=0; i<CanOutCard.cbCardTypeCount; ++i)
	{
		//��С����
		unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,CanOutCard.cbCardData[i], CanOutCard.cbEachHandCardCount[i]) ; 
		unsigned char cbMaxValue=0 ; 
		unsigned char Index = 0 ;

		//����cbMinSingleCardCount[4]�����ֵ
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


	//ԭʼ������
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
				//��Դ���
				bool bNoLargeCard = true ;

				//���������������������4�����ҳ�����С��K���ƶ��Ҳ��ǳ������ϵ������ʱ�����ܳ�2ȥ��
				if(m_cbUserCardCount[wOutCardUser]>=4 && cbHandCardCount>=5  && CanOutCard.cbEachHandCardCount[Index]>=2 && 
					GetCardLogicValue(CanOutCard.cbCardData[Index][0])>=15 &&
					GetCardLogicValue(cbTurnCardData[0])<13 &&
					(wOutCardUser==wUndersideUser&&GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]) || wOutCardUser==wUpsideUser&&GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[wUpsideUser][0])) && 
					CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
					bNoLargeCard=false ;

				//������û�д��ƣ���Էɻ���������Ĵ��ƣ�
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
			//�����
			unsigned char cbLargestLogicCard ;
			if(wOutCardUser==wUndersideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]) ;
			else if(wOutCardUser==wUpsideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUpsideUser][0]) ;
			bool bCanOut=true ;

			//��ֻѹ2
			if(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard)
			{
				if(GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0])==0x40 && GetCardLogicValue(cbTurnCardData[0])<=14 && cbHandCardCount>5) 								
				{
					bCanOut = false ;
				}
			}

			if(bCanOut)
			{
				//���ñ���
				OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
				memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

				return ;
			}
		}

		if(cbOutCardType==CT_SINGLE)
		{
			//�мҵ������
			unsigned char cbLargestLogicCard ;
			if(wOutCardUser==wUndersideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]) ;
			else if(wOutCardUser==wUpsideUser) cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUpsideUser][0]) ;

			if(GetCardLogicValue(cbTurnCardData[0])==14 || 
				GetCardLogicValue(cbTurnCardData[0])>=cbLargestLogicCard || 
				(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard-1) ||
				(wOutCardUser==wUndersideUser&&m_cbUserCardCount[wUndersideUser]<=5 || wOutCardUser==wUpsideUser&&m_cbUserCardCount[wUpsideUser]<=5))
			{
				//ȡһ�Ŵ��ڵ���2����Ҫ���мҳ����ƴ���ƣ�
				unsigned char cbIndex=MAX_COUNT ;
				for(unsigned char i=0; i<cbHandCardCount; ++i)
					if(GetCardLogicValue(cbHandCardData[i])>GetCardLogicValue(cbTurnCardData[0]) &&
						GetCardLogicValue(cbHandCardData[i])>=15)
					{
						cbIndex = i ;
					}
					if(cbIndex!=MAX_COUNT)
					{
						//���ñ���
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
		//���ñ���
		OutCardResult.cbCardCount=CardTypeResult[cbOutCardType].cbEachHandCardCount[Index];
		memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index], OutCardResult.cbCardCount) ;

		return ;
	}

	//��Ҫ����ը��
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

		//�жϳ���ը����ĵ�����
		unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]) ;
		if(cbSingleCardCount>=3 || (cbOutCardType==CT_SINGLE && GetCardLogicValue(cbTurnCardData[0])<15)) return ;

		//���ñ���
		OutCardResult.cbCardCount=BomCard.cbEachHandCardCount[Index];
		memcpy(OutCardResult.cbResultCard,BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]*sizeof(unsigned char));

		return ;
	}

	return ;
}

//�����ϼң��ȳ��ƣ�
void CAndroidAI::UpsideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wMeChairID, tagOutCardResult & OutCardResult)
{
	//���±�û��
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//��ʼ����
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

	//�����˳�ƺ͵�ֻ��һ�ԣ����ҵ�ֻ��Աȵ�����С�����ȳ�˳
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
		//˫��ը����һ��
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
	//������һֻ
	if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//����
	else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//ֻʣһ����
	else if(CT_ERROR!=GetCardType(cbHandCardData, cbHandCardCount))
	{
		OutCardResult.cbCardCount = cbHandCardCount ;
		memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount) ;
		return ;
	}

	//ֻʣһ�ź�һ��
	if(cbHandCardCount>=2)
	{
		//�����˿�
		tagOutCardTypeResult BankerCanOutCardType1[13] ;
		my_memset(BankerCanOutCardType1, sizeof(BankerCanOutCardType1)) ;
		tagOutCardTypeResult BankerCanOutCardType2[13] ;
		my_memset(BankerCanOutCardType2, sizeof(BankerCanOutCardType2)) ;

		unsigned char cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount-1) ;
		unsigned char cbSecondHandCardType = GetCardType(cbHandCardData+1, cbHandCardCount-1) ;

		//�������Գ�����
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


	//�¼�Ϊ���������ҵ����˿�����5��
	//	if(m_cbUserCardCount[m_wBankerUser]<=5)
	{
		//�����˿�
		tagOutCardTypeResult MeOutCardTypeResult[13] ;
		my_memset(MeOutCardTypeResult, sizeof(MeOutCardTypeResult)) ;
		AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;

		//�Լ��˿�
		int wFriendID ;
		for(int wChairID=0; wChairID<GAME_PLAYER; ++wChairID) 
			if(wChairID!=m_wBankerUser && wMeChairID!=wChairID) wFriendID = wChairID ;

		//���㵥��
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

		//��ը�������
		for(unsigned char cbCardType=CT_DOUBLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType] ;

			//��ͬ���ͣ���ͬ���ȣ���������������ͬ���Ϳ��ܳ��Ȳ�һ��
			unsigned char cbThisHandCardCount = MAX_COUNT ;

			//�����˿�
			tagOutCardTypeResult BankerCanOutCard[13] ;
			my_memset(BankerCanOutCard, sizeof(BankerCanOutCard)) ;

			tagOutCardTypeResult FriendOutCardTypeResult[13] ;
			my_memset(FriendOutCardTypeResult, sizeof(FriendOutCardTypeResult)) ;

			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				//���·���
				if(tmpCardResult.cbEachHandCardCount[i]!=cbThisHandCardCount)
				{
					AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], 
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,BankerCanOutCard) ;
					AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID],
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,FriendOutCardTypeResult) ;
				}

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//��������ѹ�ƣ����Ҷ��Ѳ�����ѹ����
				if((BankerCanOutCard[cbCardType].cbCardTypeCount>0&&FriendOutCardTypeResult[cbCardType].cbCardTypeCount==0) || (BankerCanOutCard[cbCardType].cbCardTypeCount>0 && FriendOutCardTypeResult[cbCardType].cbCardTypeCount>0 &&
					GetCardLogicValue(FriendOutCardTypeResult[cbCardType].cbCardData[0][0])<=GetCardLogicValue(BankerCanOutCard[cbCardType].cbCardData[0][0])))
				{
					continue ;
				}
				//�Ƿ��д���
				if(tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
				{
					bool bHaveLargeCard=false ;
					for(unsigned char j=0; j<tmpCardResult.cbEachHandCardCount[i]; ++j)
						if(GetCardLogicValue(tmpCardResult.cbCardData[i][j])>=15) bHaveLargeCard=true ;
					if(cbCardType!=CT_SINGLE_LINE && cbCardType!=CT_DOUBLE_LINE  && GetCardLogicValue(tmpCardResult.cbCardData[i][0])==14) bHaveLargeCard=true ; 

					if(bHaveLargeCard) continue ;
				}

				//�����Ƿ�����ߵ������ﶼû�п���ը��
				if(tmpCardResult.cbEachHandCardCount[i]==m_cbUserCardCount[m_wBankerUser] &&
					GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0])>GetCardLogicValue(tmpCardResult.cbCardData[i][0])) continue ;

				//����cbMinSingleCardCount[4]�����ֵ
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

				//������Сֵ
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//��С����
					cbMinCardType = cbCardType ;
					//��С�����е���С����
					cbMinSingleCountInFour=cbTmpCount ;						
					//��С�����е���С��
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

			//ѡ����С��
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//������һֻ
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//����
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//���ñ���
				OutCardResult.cbCardCount=MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,MeOutCardTypeResult[cbMinCardType].cbCardData[Index],MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}

		//��������˿�����5����û���ҵ��ʺϵ�����Ӵ����С
		if(OutCardResult.cbCardCount<=0 && m_cbUserCardCount[m_wBankerUser]<=5)
		{
			//ֻ��һ����ʱ���ܷŵ�����
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
	//��������2
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

		//���㵥��
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

		//������������
		unsigned char cbBankerSingleCardData[MAX_COUNT] ;
		unsigned char cbBankerSingleCardCount=AnalyseSinleCardCount(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], NULL, 0, cbBankerSingleCardData) ;
		unsigned char cbBankerSingleCardLogic = 0 ;
		if(cbBankerSingleCardCount>2 && GetCardLogicValue(cbBankerSingleCardData[1])<=10) cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[1]) ;
		else if(cbBankerSingleCardCount>0 && GetCardLogicValue(cbBankerSingleCardData[0])<=10) cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[0]) ;

		//��ը�������
		for(unsigned char cbCardType=CT_SINGLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType] ;
			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				//���ܷ��ߵ���С��
				if(cbCardType==CT_SINGLE && GetCardLogicValue(tmpCardResult.cbCardData[i][0])<cbBankerSingleCardLogic) continue ;

				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//����cbMinSingleCardCount[4]�����ֵ
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

				//������Сֵ
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//��С����
					cbMinCardType = cbCardType ;
					//��С�����е���С����
					cbMinSingleCountInFour=cbTmpCount ;						
					//��С�����е���С��
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//ѡ����С��
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//������һֻ
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//����
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//���ñ���
				OutCardResult.cbCardCount=CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[cbMinCardType].cbCardData[Index],CardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}
		//���ֻʣը��
		else
		{	
			unsigned char Index=0 ;
			unsigned char cbLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			//��Сը��
			for(unsigned char i=0; i<CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
				if(cbLogicCardValue>GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
				{
					cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]) ;
					Index = i ;
				}

				//���ñ���
				OutCardResult.cbCardCount=CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[CT_BOMB_CARD].cbCardData[Index],CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index]*sizeof(unsigned char));

				return ;
		}

		//�����û���������ͳ���С��һ��
		OutCardResult.cbCardCount = 1 ;
		OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount-1] ;
		return ;
}

//�����ϼң�����ƣ�
void CAndroidAI::UpsideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult) 
{

	//���±�û��
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//��ʼ����
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//��������
	unsigned char cbOutCardType = GetCardType(cbTurnCardData, cbTurnCardCount) ;

	//�����ɳ���
	tagOutCardTypeResult BankerOutCardTypeResult[13] ;
	my_memset(BankerOutCardTypeResult, sizeof(BankerOutCardTypeResult)) ;

	AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], BankerOutCardTypeResult) ;
	AnalyseOutCardType(cbHandCardData,cbHandCardCount,cbTurnCardData,cbTurnCardCount, CardTypeResult) ;

	//ֻʣը��
	if(cbHandCardCount==CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
	{
		OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0] ;
		memcpy(OutCardResult.cbResultCard,  CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount) ;

		return ;
	}
	//˫��ը����һ��
	else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}

	//���ׯ��û�д���������ѹ�Լ���
	if( m_cbUserCardCount[m_wBankerUser]<=5 && wOutCardUser!=m_wBankerUser &&
		(BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount==0 ||
		GetCardLogicValue(BankerOutCardTypeResult[cbOutCardType].cbCardData[0][0])<=GetCardLogicValue(cbTurnCardData[0])) &&
		CardTypeResult[cbOutCardType].cbEachHandCardCount[0]!=cbHandCardCount)//����һ�γ���
	{
		//��������
		return ;
	}

	//�¼�Ϊ���������ҵ����˿�����5��
	if(m_cbUserCardCount[m_wBankerUser]<=5 && CardTypeResult[cbOutCardType].cbCardTypeCount>0 && cbOutCardType!=CT_BOMB_CARD && 
		((GetCardLogicValue(cbTurnCardData[0])<12 && wOutCardUser!=m_wBankerUser && BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount>0) ||//�Լҳ���
		(wOutCardUser==m_wBankerUser)))//��������
	{
		unsigned char Index=0;
		//Ѱ�ҿ���ѹס��������Сһ����
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

	//ȡ���ĸ���С����
	unsigned char cbMinSingleCardCount[4] ;
	cbMinSingleCardCount[0]=MAX_COUNT ;
	cbMinSingleCardCount[1]=MAX_COUNT ;
	cbMinSingleCardCount[2]=MAX_COUNT ;
	cbMinSingleCardCount[3]=MAX_COUNT ;
	unsigned char cbIndex[4]={0} ;	
	unsigned char cbMinSingleCountInFour=MAX_COUNT ;

	//�ɳ��˿ˣ������Ѿ����˵�ը���ˣ�
	tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType] ;

	for(unsigned char i=0; i<CanOutCard.cbCardTypeCount; ++i)
	{
		//��С����
		unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,CanOutCard.cbCardData[i], CanOutCard.cbEachHandCardCount[i]) ; 
		unsigned char cbMaxValue=0 ; 
		unsigned char Index = 0 ;

		//����cbMinSingleCardCount[4]�����ֵ
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


	//ԭʼ������
	unsigned char cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,NULL,0) ;

	//���ѳ���
	bool bFriendOut = m_wBankerUser!=wOutCardUser ;
	if(bFriendOut)
	{
		if(CanOutCard.cbCardTypeCount>0)
		{
			//������������
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

				//С��J���ƣ�����С��K������ɢ��
				if(bCanOut ||
					((cbMinSingleCardCount[i]<cbOriginSingleCardCount+4 && cbMinSingleCardCount[i]<=cbMinSingleCountInFour && 
					(GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=11 || (cbMinSingleCardCount[i]<cbOriginSingleCardCount)&&GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=13)) &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]) && cbHandCardCount>5))
				{
					//������û�д��ƣ���Էɻ���������Ĵ��ƣ�
					bool bNoLargeCard = true ;
					for(unsigned char k=3; k<CanOutCard.cbEachHandCardCount[Index]; ++k)
					{
						//�д��ƶ��Ҳ���һ�γ���
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

				//���ñ���
				OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
				memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

				return ;
			}
			//��������������
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
						//���ñ���
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
	//��������
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
					//��Դ���
					bool bNoLargeCard = true ;

					//������������������4�����ҵ���������С��K���ƶ��Ҳ��ǵ������ϵ������ʱ�����ܳ�2ȥ��
					if(m_cbUserCardCount[m_wBankerUser]>=4 && cbHandCardCount>=5  && CanOutCard.cbEachHandCardCount[Index]>=2 && 
						GetCardLogicValue(CanOutCard.cbCardData[Index][0])>=15 &&
						GetCardLogicValue(cbTurnCardData[0])<13 &&
						GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) && 
						CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
						bNoLargeCard=false ;

					//������û�д��ƣ���Էɻ���������Ĵ��ƣ�
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
				//�����������
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;
				bool bCanOut=true ;

				//��ֻѹ2
				if(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard)
				{
					if(GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0])==0x40 && GetCardLogicValue(cbTurnCardData[0])<=14 && cbHandCardCount>5) 								
					{
						bCanOut = false ;
					}
				}

				if(bCanOut)
				{
					//���ñ���
					OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
					memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

					return ;
				}
			}

			if(cbOutCardType==CT_SINGLE)
			{
				//�����������
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;

				if(GetCardLogicValue(cbTurnCardData[0])==14 || 
					GetCardLogicValue(cbTurnCardData[0])>=cbLargestLogicCard || 
					(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard-1) ||
					m_cbUserCardCount[m_wBankerUser]<=5)
				{
					//ȡһ�Ŵ��ڵ���2����Ҫ�ȵ��������ƴ���ƣ�
					unsigned char cbIndex=MAX_COUNT ;
					for(unsigned char i=0; i<cbHandCardCount; ++i)
						if(GetCardLogicValue(cbHandCardData[i])>GetCardLogicValue(cbTurnCardData[0]) &&	GetCardLogicValue(cbHandCardData[i])>=15)
						{
							cbIndex = i ;
						}
						if(cbIndex!=MAX_COUNT)
						{
							//���ñ���
							OutCardResult.cbCardCount=1;
							OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex] ;

							return ;
						}
				}
			}
		}

		//��Ҫ����ը��
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

			//�жϳ���ը����ĵ�����
			unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]) ;
			if(cbSingleCardCount>=3 || (cbOutCardType==CT_SINGLE && GetCardLogicValue(cbTurnCardData[0])<15)) return ;

			//���ñ���
			OutCardResult.cbCardCount=BomCard.cbEachHandCardCount[Index];
			memcpy(OutCardResult.cbResultCard,BomCard.cbCardData[Index],BomCard.cbEachHandCardCount[Index]*sizeof(unsigned char));

			return ;
		}

		return ;
	}
	return ;
}

//�����¼ң��ȳ��ƣ�
void CAndroidAI::UndersideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wMeChairID,tagOutCardResult & OutCardResult) 
{
	//���±�û��
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//��ʼ����
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

	//�����˳�ƺ͵�ֻ��һ�ԣ����ҵ�ֻ��Աȵ�����С�����ȳ�˳
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
		//˫��ը����һ��
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
	//������һֻ
	if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//����
	else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}
	//ֻʣһ����
	else if(CT_ERROR!=GetCardType(cbHandCardData, cbHandCardCount))
	{
		OutCardResult.cbCardCount = cbHandCardCount ;
		memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount) ;
		return ;
	}

	//ֻʣһ�ź�һ��
	if(cbHandCardCount>=2)
	{
		//�����˿�
		tagOutCardTypeResult BankerCanOutCardType1[13] ;
		my_memset(BankerCanOutCardType1, sizeof(BankerCanOutCardType1)) ;
		tagOutCardTypeResult BankerCanOutCardType2[13] ;
		my_memset(BankerCanOutCardType2, sizeof(BankerCanOutCardType2)) ;

		unsigned char cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount-1) ;
		unsigned char cbSecondHandCardType = GetCardType(cbHandCardData+1, cbHandCardCount-1) ;

		//�������Գ�����
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


	//�¼�Ϊ���������ҵ����˿�����5��
	//	if(m_cbUserCardCount[m_wBankerUser]<=5)
	{
		//�����˿�
		tagOutCardTypeResult MeOutCardTypeResult[13] ;
		my_memset(MeOutCardTypeResult, sizeof(MeOutCardTypeResult)) ;
		AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;

		//�Լ��˿�
		int wFriendID ;
		for(int wChairID=0; wChairID<GAME_PLAYER; ++wChairID) 
			if(wChairID!=m_wBankerUser && wMeChairID!=wChairID) wFriendID = wChairID ;

		/*
		unsigned char cbFirstCard=0 ;
		//��������2
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


		//���㵥��
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

		//��ը�������
		for(unsigned char cbCardType=CT_DOUBLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType] ;

			//��ͬ���ͣ���ͬ���ȣ���������������ͬ���Ϳ��ܳ��Ȳ�һ��
			unsigned char cbThisHandCardCount = MAX_COUNT ;

			//�����˿�
			tagOutCardTypeResult BankerCanOutCard[13] ;
			my_memset(BankerCanOutCard, sizeof(BankerCanOutCard)) ;

			tagOutCardTypeResult FriendOutCardTypeResult[13] ;
			my_memset(FriendOutCardTypeResult, sizeof(FriendOutCardTypeResult)) ;

			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				//���·���
				if(tmpCardResult.cbEachHandCardCount[i]!=cbThisHandCardCount)
				{
					AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], 
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,BankerCanOutCard) ;
					AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID],
						tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i] ,FriendOutCardTypeResult) ;
				}

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//��������ѹ�ƣ����Ҷ��Ѳ�����ѹ����
				if((BankerCanOutCard[cbCardType].cbCardTypeCount>0&&FriendOutCardTypeResult[cbCardType].cbCardTypeCount==0) || (BankerCanOutCard[cbCardType].cbCardTypeCount>0 && FriendOutCardTypeResult[cbCardType].cbCardTypeCount>0 &&
					GetCardLogicValue(FriendOutCardTypeResult[cbCardType].cbCardData[0][0])<=GetCardLogicValue(BankerCanOutCard[cbCardType].cbCardData[0][0])))
				{
					continue ;
				}
				//�Ƿ��д���
				if(tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
				{
					bool bHaveLargeCard=false ;
					for(unsigned char j=0; j<tmpCardResult.cbEachHandCardCount[i]; ++j)
						if(GetCardLogicValue(tmpCardResult.cbCardData[i][j])>=15) bHaveLargeCard=true ;

					if(cbCardType!=CT_SINGLE_LINE && cbCardType!=CT_DOUBLE_LINE && GetCardLogicValue(tmpCardResult.cbCardData[i][0])==14) bHaveLargeCard=true ; 

					if(bHaveLargeCard) continue ;
				}

				//�����Ƿ�����ߵ������ﶼû�п���ը��
				if(tmpCardResult.cbEachHandCardCount[i]==m_cbUserCardCount[m_wBankerUser] &&
					GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0])>GetCardLogicValue(tmpCardResult.cbCardData[i][0])) continue ;

				//����cbMinSingleCardCount[4]�����ֵ
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

				//������Сֵ
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//��С����
					cbMinCardType = cbCardType ;
					//��С�����е���С����
					cbMinSingleCountInFour=cbTmpCount ;						
					//��С�����е���С��
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

			//ѡ����С��
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//������һֻ
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//����
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//���ñ���
				OutCardResult.cbCardCount=MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,MeOutCardTypeResult[cbMinCardType].cbCardData[Index],MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}

		//��������˿�����5����û���ҵ��ʺϵ�����Ӵ����С
		if(OutCardResult.cbCardCount<=0 && m_cbUserCardCount[m_wBankerUser]<=5)
		{
			//ֻ��һ����ʱ���ܷŵ�����
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
	//��������2
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

		//���㵥��
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

		//��ը�������
		for(unsigned char cbCardType=CT_SINGLE; cbCardType<CT_BOMB_CARD; ++cbCardType)
		{
			tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType] ;
			for(unsigned char i=0; i<tmpCardResult.cbCardTypeCount; ++i)
			{
				unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i]) ;

				unsigned char cbMaxValue=0 ; 
				unsigned char Index = 0 ;

				//����cbMinSingleCardCount[4]�����ֵ
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

				//������Сֵ
				if(cbMinSingleCountInFour>=cbTmpCount)
				{
					//��С����
					cbMinCardType = cbCardType ;
					//��С�����е���С����
					cbMinSingleCountInFour=cbTmpCount ;						
					//��С�����е���С��
					cbMinIndex=i ;
				}
			}
		}

		if(cbMinSingleCountInFour!=MAX_COUNT)
		{
			unsigned char Index = cbMinIndex ;

			//ѡ����С��
			for(unsigned char i=0; i<4; ++i)
			{
				if(cbOutcardType[i]==cbMinCardType && cbMinSingleCardCount[i]<=cbMinSingleCountInFour &&
					GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0])<GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
					Index = cbIndex[i] ;
			}

			//������һֻ
			if(cbHandCardCount==3 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			//����
			else if(cbHandCardCount==2 && GetCardColor(cbHandCardData[0])==0x40 && GetCardColor(cbHandCardData[1])==0x40)
			{
				OutCardResult.cbCardCount = 2 ;
				OutCardResult.cbResultCard[0] = 0x4f ;
				OutCardResult.cbResultCard[1] = 0x4e ;
				return ;
			}
			else
			{
				//���ñ���
				OutCardResult.cbCardCount=CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[cbMinCardType].cbCardData[Index],CardTypeResult[cbMinCardType].cbEachHandCardCount[Index]*sizeof(unsigned char));
				return ;
			}

			ASSERT(OutCardResult.cbCardCount>0) ;

			return ;
		}
		//���ֻʣը��
		else
		{	
			unsigned char Index=0 ;
			unsigned char cbLogicCardValue = GetCardLogicValue(0x4F)+1 ;
			//��Сը��
			for(unsigned char i=0; i<CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
				if(cbLogicCardValue>GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
				{
					cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]) ;
					Index = i ;
				}

				//���ñ���
				OutCardResult.cbCardCount=CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard,CardTypeResult[CT_BOMB_CARD].cbCardData[Index],CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index]*sizeof(unsigned char));

				return ;
		}

		//�����û���������ͳ���С��һ��
		OutCardResult.cbCardCount = 1 ;
		OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount-1] ;
		return ;
}
//�����¼ң�����ƣ�
void CAndroidAI::UndersideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult)
{
	//��ʼ����
	my_memset(&OutCardResult, sizeof(OutCardResult)) ;

	//���±�û��
	tagOutCardTypeResult CardTypeResult[12+1] ;
	my_memset(CardTypeResult, sizeof(CardTypeResult)) ;

	//��������
	unsigned char cbOutCardType = GetCardType(cbTurnCardData,cbTurnCardCount) ;

	//�����ɳ���
	tagOutCardTypeResult BankerOutCardTypeResult[13] ;
	my_memset(BankerOutCardTypeResult, sizeof(BankerOutCardTypeResult)) ;

	AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], BankerOutCardTypeResult) ;
	AnalyseOutCardType(cbHandCardData,cbHandCardCount,cbTurnCardData,cbTurnCardCount, CardTypeResult) ;

	//ֻʣը��
	if(cbHandCardCount==CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
	{
		OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0] ;
		memcpy(OutCardResult.cbResultCard,  CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount) ;

		return ;
	}
	//˫��ը����һ��
	else if(cbHandCardCount>2 && cbHandCardData[0]==0x4f && cbHandCardData[1]==0x4e && CT_ERROR!=GetCardType(cbHandCardData+2, cbHandCardCount-2))
	{
		OutCardResult.cbCardCount = 2 ;
		OutCardResult.cbResultCard[0] = 0x4f ;
		OutCardResult.cbResultCard[1] = 0x4e ;
		return ;
	}

	//ȡ���ĸ���С����
	unsigned char cbMinSingleCardCount[4] ;
	cbMinSingleCardCount[0]=MAX_COUNT ;
	cbMinSingleCardCount[1]=MAX_COUNT ;
	cbMinSingleCardCount[2]=MAX_COUNT ;
	cbMinSingleCardCount[3]=MAX_COUNT ;
	unsigned char cbIndex[4]={0} ;	
	unsigned char cbMinSingleCountInFour=MAX_COUNT ;

	//�ɳ��˿ˣ������Ѿ����˵�ը���ˣ�
	tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType] ;

	for(unsigned char i=0; i<CanOutCard.cbCardTypeCount; ++i)
	{
		//��С����
		unsigned char cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,CanOutCard.cbCardData[i], CanOutCard.cbEachHandCardCount[i]) ; 
		unsigned char cbMaxValue=0 ; 
		unsigned char Index = 0 ;

		//����cbMinSingleCardCount[4]�����ֵ
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


	//ԭʼ������
	unsigned char cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,NULL,0) ;

	//���ѳ���
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
				//С��J���ƣ�����С��K������ɢ��
				if((cbMinSingleCardCount[i]<cbOriginSingleCardCount+4 && cbMinSingleCardCount[i]<=cbMinSingleCountInFour && 
					(GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=11 || (cbMinSingleCardCount[i]<cbOriginSingleCardCount)&&GetCardLogicValue(CanOutCard.cbCardData[Index][0])<=13)) &&
					cbMinLogicCardValue>GetCardLogicValue(CanOutCard.cbCardData[Index][0]) && cbHandCardCount>5)
				{
					//������û�д��ƣ���Էɻ���������Ĵ��ƣ�
					bool bNoLargeCard = true ;
					for(unsigned char k=3; k<CanOutCard.cbEachHandCardCount[Index]; ++k)
					{
						//�д��ƶ��Ҳ���һ�γ���
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

				//���ñ���
				OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
				memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

				return ;
			}
			//��������������
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
						//���ñ���
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
	//��������
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
					//��Դ���
					bool bNoLargeCard = true ;

					//������������������4�����ҵ���������С��K���ƶ��Ҳ��ǵ������ϵ������ʱ�����ܳ�2ȥ��
					if(m_cbUserCardCount[m_wBankerUser]>=4 && cbHandCardCount>=5 && CanOutCard.cbEachHandCardCount[Index]>=2 && 
						GetCardLogicValue(CanOutCard.cbCardData[Index][0])>=15 &&
						GetCardLogicValue(cbTurnCardData[0])<13 &&
						GetCardLogicValue(cbTurnCardData[0])<GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) && 
						CanOutCard.cbEachHandCardCount[Index]!=cbHandCardCount)
						bNoLargeCard=false ;

					//������û�д��ƣ���Էɻ���������Ĵ��ƣ�
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
				//�����������
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;
				bool bCanOut=true ;

				//��ֻѹ2
				if(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard)
				{
					if(GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0])==0x40 && GetCardLogicValue(cbTurnCardData[0])<=14 && cbHandCardCount>5) 								
					{
						bCanOut = false ;
					}
				}

				if(bCanOut)
				{
					//���ñ���
					OutCardResult.cbCardCount=CanOutCard.cbEachHandCardCount[cbCanOutIndex];
					memcpy(OutCardResult.cbResultCard,CanOutCard.cbCardData[cbCanOutIndex],CanOutCard.cbEachHandCardCount[cbCanOutIndex]*sizeof(unsigned char));

					return ;
				}
			}

			if(cbOutCardType==CT_SINGLE)
			{
				//�����������
				unsigned char cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) ;

				if(GetCardLogicValue(cbTurnCardData[0])==14 || 
					GetCardLogicValue(cbTurnCardData[0])>=cbLargestLogicCard || 
					(GetCardLogicValue(cbTurnCardData[0])<cbLargestLogicCard-1) ||
					m_cbUserCardCount[m_wBankerUser]<=5)
				{
					//ȡһ�Ŵ��ڵ���2����Ҫ�ȵ��������ƴ���ƣ�
					unsigned char cbIndex=MAX_COUNT ;
					for(unsigned char i=0; i<cbHandCardCount; ++i)
						if(GetCardLogicValue(cbHandCardData[i])>GetCardLogicValue(cbTurnCardData[0]) &&
							GetCardLogicValue(cbHandCardData[i])>=15)
						{
							cbIndex = i ;
						}
						if(cbIndex!=MAX_COUNT)
						{
							//���ñ���
							OutCardResult.cbCardCount=1;
							OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex] ;

							return ;
						}
				}
			}

			//�����Ѳ������ص���ʱ
			int wMeChairID = (m_wBankerUser+1)%GAME_PLAYER ;
			int wFriendID = (wMeChairID+1)%GAME_PLAYER ;

			tagOutCardTypeResult FriendCardTypeResult[13] ;
			my_memset(FriendCardTypeResult, sizeof(FriendCardTypeResult)) ;
			AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID], cbTurnCardData, cbTurnCardCount, FriendCardTypeResult) ;

			//�����Ѳ������ص���ʱ
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
				//���ñ���
				OutCardResult.cbCardCount=CardTypeResult[cbOutCardType].cbEachHandCardCount[Index];
				memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index], OutCardResult.cbCardCount) ;

				return ;
			}
		}

		//��Ҫ����ը��
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

			//�жϳ���ը����ĵ�����
			unsigned char cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, BomCard.cbCardData[Index], BomCard.cbEachHandCardCount[Index]) ;
			if(cbSingleCardCount>=3 || (cbOutCardType==CT_SINGLE && GetCardLogicValue(cbTurnCardData[0])<15)) return ;

			//���ñ���
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
	//�����˿�
	for (int wChairID = 0; wChairID < GAME_PLAYER; ++wChairID){
		SetUserCard(wChairID, &data->cardData[wChairID * NORMAL_COUNT], NORMAL_COUNT);
	}
}
