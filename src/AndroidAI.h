#ifndef _ANDROIDAI_H_
#define _ANDROIDAI_H_

#include "GameLogic.h"
#include <stdlib.h>
#include <memory.h>

class CAndroidAI
{
public:
	CAndroidAI(void);
	~CAndroidAI(void);

public:
	void SetUserCard(int wChairID, unsigned char cbCardData[], unsigned char cbCardCount);
	void SetBackCard(int wChairID, unsigned char cbBackCardData[], unsigned char cbCardCount);
	void SetBanker(int wBanker) ;
	void RemoveUserCardData(int wChairID, unsigned char cbRemoveCardData[], unsigned char cbRemoveCardCount);
	void ClearAllCard();
	void onGameStart(PokerData* data);
public:
	bool CompareCard(const unsigned char cbFirstCard[], const unsigned char cbNextCard[], unsigned char cbFirstCount, unsigned char cbNextCount);
	void SortCardList(unsigned char cbCardData[], unsigned char cbCardCount, unsigned char cbSortType);
	bool RemoveCard(const unsigned char cbRemoveCard[], unsigned char cbRemoveCount, unsigned char cbCardData[], unsigned char cbCardCount);
	unsigned char GetCardType(const unsigned char cbCardData[], unsigned char cbCardCount);
public:
	void Combination(unsigned char cbCombineCardData[], unsigned char cbResComLen,  unsigned char cbResultCardData[254][5], unsigned char &cbResCardLen,unsigned char cbSrcCardData[] , unsigned char cbCombineLen1, unsigned char cbSrcLen, const unsigned char cbCombineLen2);
	void Permutation(unsigned char *list, int m, int n, unsigned char result[][4], unsigned char &len) ;
	void GetAllBomCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbBomCardData[], unsigned char &cbBomCardCount);
	void GetAllLineCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbLineCardData[], unsigned char &cbLineCardCount);
	void GetAllThreeCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbThreeCardData[], unsigned char &cbThreeCardCount);
	void GetAllDoubleCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbDoubleCardData[], unsigned char &cbDoubleCardCount);
	void GetAllSingleCard(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char cbSingleCardData[], unsigned char &cbSingleCardCount);
public:
	void AnalyseOutCardType(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char const cbTurnCardData[], unsigned char const cbTurnCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
	void AnalyseOutCardType(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
	unsigned char AnalyseSinleCardCount(unsigned char const cbHandCardData[], unsigned char const cbHandCardCount, unsigned char const cbWantOutCardData[], unsigned char const cbWantOutCardCount, unsigned char cbSingleCardData[]=NULL);
public:
	void BankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, tagOutCardResult & OutCardResult) ;
	void BankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult) ;
	void UpsideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wMeChairID,tagOutCardResult & OutCardResult) ;
	void UpsideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser,  const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult) ;
	void UndersideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wMeChairID,tagOutCardResult & OutCardResult) ;
	void UndersideOfBankerOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, int wOutCardUser, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, tagOutCardResult & OutCardResult) ;
	bool SearchOutCard(const unsigned char cbHandCardData[], unsigned char cbHandCardCount, const unsigned char cbTurnCardData[], unsigned char cbTurnCardCount, int wOutCardUser, int wMeChairID, tagOutCardResult & OutCardResult);
	unsigned char AnalyseLandScore(int chairIdx, unsigned char cbBackCardData[]);
public:
	unsigned char GetCardValue(unsigned char cbCardData) { return cbCardData&MASK_VALUE; }
	unsigned char GetCardColor(unsigned char cbCardData) { return cbCardData&MASK_COLOR; }
	bool IsValidCard(unsigned char cbCardData);
	unsigned char GetCardLogicValue(unsigned char cbCardData);
	void AnalysebCardData(const unsigned char cbCardData[], unsigned char cbCardCount, tagAnalyseResult & AnalyseResult);
public:
	unsigned char m_cbAllCardData[GAME_PLAYER][MAX_COUNT];
	unsigned char m_cbUserCardCount[GAME_PLAYER];

	int m_wBankerUser;
	long m_lBankerOutCardCount ;
};


#endif
