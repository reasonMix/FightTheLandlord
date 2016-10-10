#include "lua_landlord.hpp"
#include "GameLogic.h"
#include "AndroidAI.h"
#include <vector>

static CGameLogic* sCGameLogic = NULL;
static CAndroidAI sCAndroidAI;
std::vector<PokerData*> allPokerData;

/*
static int luaGetStringFromFile(lua_State* L){
	std::string fileName = lua_tostring(L, 1);
	int size;
	std::string ret;
	unsigned char* data = FileUtils::getInstance()->getFileData(fileName.c_str(), "rb", &size);
	if (data){
		ret.assign((char*)data, size);
		delete[] data;
	}

	lua_pushstring(L, ret.c_str());

	return 1;
}*/

static int initGameLogic(lua_State* L){
	if (!sCGameLogic){
		sCGameLogic = new CGameLogic();
	}

	return 0;
}

static int destroyGameLogic(lua_State* L){
	if (sCGameLogic){
		delete sCGameLogic;
		sCGameLogic = NULL;
	}

	return 0;
}

static int newPokerData(lua_State* L){
	PokerData* data = new PokerData();
	lua_pushlightuserdata(L, data);
	allPokerData.push_back(data);
	return 1;
}

static int deletePokerData(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);
	if (data){
		delete data;
	}
	return 0;
}

static int RandCardList(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);
	sCGameLogic->RandCardList(data->cardData, FULL_COUNT);

	// ����3�����ǵ�����
	memcpy(data->cbBackCardData, &data->cardData[FULL_COUNT - BACK_COUNT], BACK_COUNT);
	return 0;
}

static int getCardData(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);

	lua_newtable(L);
	for (int i = 1; i <= FULL_COUNT; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushinteger(L, data->cardData[i-1]);
		lua_rawset(L, -3);
	}
	return 1;
}

static int getBackCard(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);

	lua_newtable(L);
	for (int i = 1; i <= BACK_COUNT; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushinteger(L, data->cbBackCardData[i - 1]);
		lua_rawset(L, -3);
	}
	return 1;
}

static int getUserCard(lua_State* L){
	CAndroidAI* data = (CAndroidAI*)lua_touserdata(L, 1);
	int userIdx = lua_tointeger(L, 2);

	lua_newtable(L);
	for (int i = 1; i <= data->m_cbUserCardCount[userIdx - 1]; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushinteger(L, data->m_cbAllCardData[userIdx - 1][i - 1]);
		lua_rawset(L, -3);
	}
	return 1;
}

static int GetCardColor(lua_State* L){
	int card = lua_tointeger(L, 1);
	int color = sCAndroidAI.GetCardColor(card);
	color = color >> 4;
	lua_pushinteger(L, color);

	return 1;
}

static int GetCardLogicValue(lua_State* L){
	int card = lua_tointeger(L, 1);
	int value = sCAndroidAI.GetCardLogicValue(card);
	lua_pushinteger(L, value);

	return 1;
}

static int newAndroidAI(lua_State* L){
	CAndroidAI* ai = new CAndroidAI();
	lua_pushlightuserdata(L, ai);
	return 1;
}

static int deleteAndroidAI(lua_State* L){
	CAndroidAI* ai = (CAndroidAI*)lua_touserdata(L, 1);
	delete ai;
	return 0;
}

static int onGameStart(lua_State* L){
	CAndroidAI* ai = (CAndroidAI*)lua_touserdata(L, 1);
	PokerData* data = (PokerData*)lua_touserdata(L, 2);
	ai->onGameStart(data);

	return 0;
}

static int luaAnalyseLandScore(lua_State* L){
	CAndroidAI* ai = (CAndroidAI*)lua_touserdata(L, 1);
	PokerData* data = (PokerData*)lua_touserdata(L, 2);
	int chairIdx = lua_tointeger(L, 3);
	chairIdx = chairIdx - 1;

	int ret = ai->AnalyseLandScore(chairIdx, data->cbBackCardData);
	lua_pushinteger(L, ret);

	return 1;
}

static int luaUpdateTurnData(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);
	int outPokerUser = lua_tointeger(L, 2);
	data->outPokerUser = outPokerUser - 1;

	// table
	size_t len = lua_objlen(L, 3);
	data->m_cbTurnCardCount = len;

	for (size_t i = 0; i < len; i++)
	{
		lua_pushnumber(L, i + 1);
		lua_gettable(L, 3);

		data->m_cbTurnCardData[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}

	return 0;
}

static int luagetOutPokerIdx(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);
	lua_pushinteger(L,data->outPokerUser);

	return 1;
}

static int luaSearchOutCard(lua_State* L){
	CAndroidAI* ai = (CAndroidAI*)lua_touserdata(L, 1);
	PokerData* data = (PokerData*)lua_touserdata(L, 2);
	int playerIdx = lua_tointeger(L, 3) - 1;
	int outPokerUser = lua_tointeger(L, 4);
	tagOutCardResult result;

	ai->SearchOutCard(ai->m_cbAllCardData[playerIdx], ai->m_cbUserCardCount[playerIdx],
		data->m_cbTurnCardData, data->m_cbTurnCardCount, outPokerUser, playerIdx, result);

	ai->RemoveUserCardData(playerIdx, result.cbResultCard, result.cbCardCount);

	lua_newtable(L);
	for (int i = 1; i <= result.cbCardCount; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushinteger(L, result.cbResultCard[i - 1]);
		lua_rawset(L, -3);
	}

	return 1;
}

static int luaGetCardType(lua_State* L){
	size_t len = lua_objlen(L, 1);
	unsigned char cardData[MAX_COUNT];

	for (size_t i = 0; i < len; i++)
	{
		lua_pushnumber(L, i + 1);
		lua_gettable(L, 1);

		cardData[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}

	sCAndroidAI.SortCardList(cardData, len, ST_ORDER);
	int type = sCAndroidAI.GetCardType(cardData,len);
	lua_pushinteger(L,type);

	return 1;
}

static int luaCompareCard(lua_State* L){
	size_t f_len = lua_objlen(L, 1);
	unsigned char cbFirstCard[MAX_COUNT];

	for (size_t i = 0; i < f_len; i++)
	{
		lua_pushnumber(L, i + 1);
		lua_gettable(L, 1);

		cbFirstCard[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}


	size_t n_len = lua_objlen(L, 2);
	unsigned char cbNextCard[MAX_COUNT];

	for (size_t i = 0; i < n_len; i++)
	{
		lua_pushnumber(L, i + 1);
		lua_gettable(L, 2);

		cbNextCard[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}
	sCAndroidAI.SortCardList(cbFirstCard, f_len, ST_ORDER);
	sCAndroidAI.SortCardList(cbNextCard, n_len, ST_ORDER);
	bool flag = sCAndroidAI.CompareCard(cbFirstCard, cbNextCard, f_len, n_len);
	lua_pushboolean(L,flag ? 1 : 0);

	return 1;
}

static int luaSetBankerUser(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);
	CAndroidAI* ai = (CAndroidAI*)lua_touserdata(L, 2);
	int idx = lua_tointeger(L, 3) - 1;
	ai->SetBanker(idx);
	data->outPokerUser = idx;

	ai->SetBackCard(ai->m_wBankerUser, data->cbBackCardData, BACK_COUNT);

	return 0;
}

static int luaRemoveUserCardData(lua_State* L){
	CAndroidAI* ai = (CAndroidAI*)lua_touserdata(L, 1);
	int playerIdx = lua_tointeger(L, 2) - 1;
	size_t n_len = lua_objlen(L, 3);
	unsigned char cbRmCard[MAX_COUNT];

	for (size_t i = 0; i < n_len; i++)
	{
		lua_pushnumber(L, i + 1);
		lua_gettable(L, 3);

		cbRmCard[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}

	ai->RemoveUserCardData(playerIdx, cbRmCard, n_len);
	return 0;
}

static int luagetLeftPoker(lua_State* L){
	CAndroidAI* ai = (CAndroidAI*)lua_touserdata(L, 1);
	int idx = lua_tointeger(L, 2) - 1;
	lua_pushinteger(L, ai->m_cbUserCardCount[idx]);

	return 1;
}

static int luagetTrunData(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);
	lua_newtable(L);
	for (int i = 1; i <= data->m_cbTurnCardCount; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushinteger(L, data->m_cbTurnCardData[i - 1]);
		lua_rawset(L, -3);
	}
	return 1;
}

static int luaclearTrunData(lua_State* L){
	PokerData* data = (PokerData*)lua_touserdata(L, 1);
	int idx = lua_tointeger(L, 2) - 1;

	memset(data->m_cbTurnCardData, 0, MAX_COUNT);
	data->m_cbTurnCardCount = 0;

	data->outPokerUser = idx;
	return 0;
}

static int luaAnalyseOutCardType(lua_State* L){
	// hand cards
	size_t HandCardCnt = lua_objlen(L, 1);
	unsigned char cbHandCardData[MAX_COUNT];

	for (size_t i = 0; i < HandCardCnt; i++)
	{
		lua_pushnumber(L, i + 1);
		lua_gettable(L, 1);

		cbHandCardData[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}

	size_t turnCardCnt = lua_objlen(L, 2);
	unsigned char cbTurnCardData[MAX_COUNT];

	for (size_t i = 0; i < turnCardCnt; i++)
	{
		lua_pushnumber(L, i + 1);
		lua_gettable(L, 2);

		cbTurnCardData[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}
	tagOutCardTypeResult CardTypeResult[12 + 1];
	memset(CardTypeResult,0, sizeof(CardTypeResult[0]) * 13);

	if (turnCardCnt > 0){
		sCAndroidAI.AnalyseOutCardType(cbHandCardData, HandCardCnt, cbTurnCardData, turnCardCnt, CardTypeResult);
	}
	else{
		sCAndroidAI.AnalyseOutCardType(cbHandCardData, HandCardCnt, CardTypeResult);
	}

	lua_newtable(L);
	for (int i = 1; i <= 13; ++i)
	{
		lua_pushnumber(L, i);
		lua_newtable(L);

		lua_pushstring(L, "cbCardType");                             /* L: table key */
		lua_pushnumber(L, CardTypeResult[i - 1].cbCardType);
		lua_rawset(L, -3);											 /* table[key] = value, L: table */

		lua_pushstring(L, "cbCardTypeCount");                             /* L: table key */
		lua_pushnumber(L, CardTypeResult[i - 1].cbCardTypeCount);
		lua_rawset(L, -3);											 /* table[key] = value, L: table */

		lua_pushstring(L, "cbEachHandCardCount");
		lua_newtable(L);
		for (int k = 1; k <= MAX_TYPE_COUNT;k++){
			lua_pushnumber(L, k);
			lua_pushnumber(L, CardTypeResult[i - 1].cbEachHandCardCount[k  - 1]);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);

		lua_pushstring(L, "cbCardData");
		lua_newtable(L);
		for (int k = 1; k <= MAX_TYPE_COUNT; k++){
			lua_pushnumber(L, k);
			lua_newtable(L);
			for (int m = 1; m <= MAX_COUNT;m++){
				lua_pushnumber(L, m);
				lua_pushnumber(L, CardTypeResult[i - 1].cbCardData[k - 1][m - 1]);
				lua_rawset(L, -3);
			}
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);

		lua_rawset(L, -3);
	}

	return 1;
}

static luaL_Reg api[] = {
	{ "initGameLogic", initGameLogic },
	{ "destroyGameLogic", destroyGameLogic },
	{ "newPokerData", newPokerData },
	{ "deletePokerData", deletePokerData },
	{ "RandCardList", RandCardList },
	{ "getCardData", getCardData },
	{ "getBackCard", getBackCard },
	{ "getUserCard", getUserCard },

	{ "GetCardColor", GetCardColor },
	{ "GetCardLogicValue", GetCardLogicValue },
	{ "newAndroidAI", newAndroidAI },
	{ "deleteAndroidAI", deleteAndroidAI },
	{ "onGameStart", onGameStart },
	{ "AnalyseLandScore", luaAnalyseLandScore },
	{ "SearchOutCard", luaSearchOutCard },
	{ "updateTurnData", luaUpdateTurnData },
	{ "getOutPokerIdx", luagetOutPokerIdx },
	{ "GetCardType", luaGetCardType },
	{ "CompareCard", luaCompareCard },
	{ "setBankerUser", luaSetBankerUser },
	{ "clearTrunData", luaclearTrunData },
	{ "RemoveUserCardData", luaRemoveUserCardData },
	{ "getTrunData", luagetTrunData },
	{ "getLeftPoker", luagetLeftPoker },
	{ "AnalyseOutCardType", luaAnalyseOutCardType },
	{ NULL, NULL },
};

int luaopen_landlord(lua_State *L)
{
	// register the mongo api
	lua_newtable(L);
	luaL_register(L, NULL, api);

	return 1;
}
