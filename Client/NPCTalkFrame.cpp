/********************************************************************
	created:	2008/08/16
	created:	16:8:2008   17:21
	filename: 	d:\loong_client\Client\NPCTalkFrame.cpp
	file path:	d:\loong_client\Client
	file base:	NPCTalkFrame
	file ext:	cpp
	author:		leili
	
	purpose:	普通NPC对话界面实现，处理窗口的界面逻辑
*********************************************************************/
#include "StdAfx.h"
#include "NPCTalkFrame.h"
#include "QuestMgr.h"
#include "ComposeMgr.h"
#include "RoleMgr.h"
#include "LocalPlayer.h"
#include "NPC.h"
#include "ItemFrame.h"
#include "CombatEvent.h"
#include "..\WorldDefine\msg_map.h"
#include "Container.h"
#include "CombatSysUtil.h"
#include "ItemMgr.h"
#include "ListFrame.h"
#include "OverSeaData.h"
#include "DresserFrame.h"

const DWORD PEARL_NECKLACE = 3301056;
//-----------------------------------------------------------------------------
// 构造函数
//-----------------------------------------------------------------------------
NPCTalkFrame::NPCTalkFrame( void )
: m_Trunk(this)
, m_pWnd(0)
, m_pStcCondition(0)
, m_pListOptions(0)
, m_pBtnCloseWnd(0)
, m_curRow(0)
, m_pCaption(0)
, m_escapeTime(0)
, m_targetType(0)
, m_targetID(GT_INVALID)
, m_isUsingTransportItem(false)
{

}

//-----------------------------------------------------------------------------
// 析构函数
//-----------------------------------------------------------------------------
NPCTalkFrame::~NPCTalkFrame( void )
{
	
}

//-----------------------------------------------------------------------------
// 初始化
//-----------------------------------------------------------------------------
BOOL NPCTalkFrame::Init( GameFrameMgr* pMgr, GameFrame* pFather, DWORD dwParam )
{
    m_isUsingTransportItem = false;

	if( !GameFrame::Init(pMgr, pFather, dwParam) )
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// 销毁
//-----------------------------------------------------------------------------
BOOL NPCTalkFrame::Destroy()
{
	GameFrame::Destroy();

	RoleMgr::Inst()->DestroyVoteUI();
	if( P_VALID(m_pWnd) )
		m_pGUI->AddToDestroyList(m_pWnd);
	m_pWnd = NULL;

	return TRUE;
}

//-----------------------------------------------------------------------------
// 重新读入窗口界面
//-----------------------------------------------------------------------------
BOOL NPCTalkFrame::ReloadUI()
{
	XmlElement element;
	tstring strPath = vEngine::g_strLocalPath + _T("\\ui\\npctalk.xml");
	if( !m_pGUI->LoadXml( &element, "VFS_System", strPath.c_str() ) )
		return FALSE;

	m_pWnd = m_pGUI->CreateWnd( _T("\\desktop"), &element );

	// 注册事件处理函数
	m_pGUI->RegisterEventHandler(m_pWnd->GetFullName().c_str(), m_Trunk.sfp1(&NPCTalkFrame::EventHandler));

	m_pCaption		= (GUIPatch*)		m_pWnd->GetChild( _T("nt_pic") );
	m_pStcCondition	= (GUIStaticEx*)	m_pWnd->GetChild( _T("nt_pic\\nt_pic2\\npctalkpic\\contentWnd\\npctalk") );
	m_pListOptions	= (GUIListBox*)		m_pWnd->GetChild( _T("nt_pic\\nt_pic2\\optpic\\optlist") );
	m_pBtnCloseWnd	= (GUIButton*)		m_pWnd->GetChild( _T("nt_pic\\new0") );

	m_pListOptions->SetColNum(1, 39);

	m_pWnd->SetInvisible( TRUE );

	return TRUE;
}

//-----------------------------------------------------------------------------
// 界面消息事件处理
//-----------------------------------------------------------------------------
BOOL NPCTalkFrame::EventHandler( tagGUIEvent* pEvent )
{
	GUIWnd* pWnd = m_pGUI->GetWnd(pEvent->dwWndID);

	switch( pEvent->eEvent )
	{
	case EGUIE_Click:
		{
			if( pWnd == m_pBtnCloseWnd )//关闭窗口
			{
				Hide();

				m_pMgr->AddToDestroyList(this);
			}
		}
		break;

	case EGUIE_ItemClick:
		{
			if( pWnd == m_pListOptions )
			{
				if( m_curRow > pEvent->dwParam1 )
				{
					QuestScript *pQS = QuestMgr::Inst()->GetScript();
					if( !P_VALID(pQS) )
						return TRUE;

					const tagOptAttr& optAttr = m_optAttrs[pEvent->dwParam1];
					switch( optAttr.type )
					{
					case EOT_Vote:
						{
							switch( optAttr.id )
							{
							case 0:
								{
									ListFrame *pFrame = (ListFrame*)TObjRef<GameFrameMgr>()->GetFrame(_T("DragonVote")); 
									if( !P_VALID(pFrame) )
									{
										pFrame = (ListFrame*)TObjRef<GameFrameMgr>()->CreateFrame( _T("World"), _T("DragonVote"), _T("ListFrame"), 0 );
									}
									tagNC_QueryDragonGirlRankings msg;
									TObjRef<NetSession>()->Send(&msg);
								}
								break;
							case 1:
								{
									ListFrame *pFrame = (ListFrame*)TObjRef<GameFrameMgr>()->GetFrame(_T("DragonVote")); 
									if( !P_VALID(pFrame) )
									{
										pFrame = (ListFrame*)TObjRef<GameFrameMgr>()->CreateFrame( _T("World"), _T("DragonVote"), _T("ListFrame"), 0 );
									}
									tagNC_QueryDragonDefenderRankings msg;
									TObjRef<NetSession>()->Send(&msg);
								}
								break;
							case 2:
								{
									ItemContainer* pContainer = ItemMgr::Inst()->GetPocket();
									INT nNum = pContainer->GetItemQuantity(PEARL_NECKLACE);
									if( nNum > 0 )
										RoleMgr::Inst()->PopUpVoteUI();
									else
										CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("CanNotVote")]);
								}
								break;
							default:
								break;
							}
						}
						break;
					case EOT_AcceptQuest:
						{
							pQS->OnAcceptQuestTalk(optAttr.id, optAttr.step);
						}
						break;
					case EOT_CompleteQuest:
						{
							pQS->OnCompleteQuestTalk(optAttr.id, optAttr.step);
						}
						break;
					case EOT_CompleteQuestBySpecItem:
						{
							pQS->OnCompleteBySpecItemQuestTalk(optAttr.id, optAttr.step);
						}
						break;
					case EOT_ScenarioTalk:
						{
							pQS->OnScenarioTalk(optAttr.id, optAttr.step);
						}
						break;
					case EOT_ProduceSkillTalk:
						{
							pQS->OnProduceSkillTalk(optAttr.id, optAttr.step);
						}
						break;
					case EOT_LearnProduceSkill:
						{
							ComposeMgr::Inst()->LearnProduceSkill(optAttr.step, QuestMgr::Inst()->GetTalkNPCGlobalID());
							Hide();
						}
						break;
					case EOT_LeaveProduceSkill:
						{
							ComposeMgr::Inst()->LeaveProduceSkill(optAttr.step, QuestMgr::Inst()->GetTalkNPCGlobalID());
						}
						break;
					case EOT_CancelProduceSkill:
						{
							ClearOptions();
							QuestMgr::Inst()->OnNPCTalk( QuestMgr::Inst()->GetTalkNPCGlobalID(),
								QuestMgr::Inst()->GetTalkNPCTypeID() );
						}
						break;
					case EOT_GuildCommerce:
						{
							pQS->OnGuildCommerceTalk(optAttr.id, optAttr.step);
							
						}
						break;
					case EOT_CityProductivity:
						{
							pQS->OnCityProductivityTalk(optAttr.id, optAttr.step);
						}
						break;
					case EOT_ServerCtrl:	//服务器控制选项，点击时需要向服务器发送消息
						{
							tagNC_DlgDefaultMsg msg;
							msg.eDlgTarget = (EMsgUnitType)m_targetType;
							msg.dwTargetID = m_targetID;
							msg.eDlgOption = (EDlgOption)optAttr.step;
							TObjRef<NetSession>()->Send(&msg);
 
							ListFrame *pFrame = (ListFrame*)TObjRef<GameFrameMgr>()->GetFrame(_T("DragonVote")); 
							if( !P_VALID(pFrame) )
							{
								pFrame = (ListFrame*)TObjRef<GameFrameMgr>()->CreateFrame( _T("World"), _T("DragonVote"), _T("ListFrame"), 0 );
							}
							Hide();
						}
						break;
                    case EOT_SetTransportPoint:
                        {
                            if (0 == optAttr.id)
                            {
                                // 定位
                                tagNC_SetTransportMap msg;
                                if (P_VALID(m_pTransportItem))
                                    msg.n64ItemID   = m_pTransportItem->GetItemId();
                                TObjRef<NetSession>()->Send(&msg);
                            }
                            else if (1 == optAttr.id)
                            {
                                // 传送
                                tagNC_UseTransportMap msg;
                                if (P_VALID(m_pTransportItem))
                                {
                                    msg.n64ItemID   = m_pTransportItem->GetItemId();
                                }
                                TObjRef<NetSession>()->Send(&msg);
                            }

                            Hide();
                        }
                        break;
					case EOT_GodEvilRepute:
						{
							if (optAttr.id == 0)
								TObjRef<GameFrameMgr>()->SendEvent(&tagGameEvent(_T("OpenGodReputeFrame"), NULL));
							else
								TObjRef<GameFrameMgr>()->SendEvent(&tagGameEvent(_T("OpenEvilReputeFrame"), NULL));

							Hide();
						}
						break;
					case EOT_God:
						{
							if(optAttr.id == 0)
							{
								//晋升神职
								TObjRef<GameFrameMgr>()->SendEvent(&tagGameEvent(_T("OpenClergyFrame"), NULL));
							}
							else if(optAttr.id == 1)
							{
								//竞选神职
								TObjRef<GameFrameMgr>()->SendEvent(&tagGameEvent(_T("OpenCampaingFrame"), NULL));
							}
							else if(optAttr.id == 2)
							{
								//查询上期结果
								TObjRef<GameFrameMgr>()->SendEvent(&tagGameEvent(_T("OpenCampaingResultFrame"), NULL));
							}
							else if(optAttr.id == 3)
							{
								//查看神之系
								TObjRef<GameFrameMgr>()->SendEvent(&tagGameEvent(_T("OpenGodRelation"), NULL));
							}
							else if(optAttr.id == 4)
							{
								//神之血脉
								TObjRef<GameFrameMgr>()->SendEvent(&tagGameEvent(_T("OpenPulse"), NULL));
							}
							else if(optAttr.id == 5)
							{
								//取消
							}
							Hide();
						}
						break;
					case EOT_ChangeFace:
						{
							if(optAttr.id == 0)
							{
								LocalPlayer* pLp = RoleMgr::Inst()->GetLocalPlayer();
								if (0 != static_cast<AvatarEquipNode*>(pLp->GetSceneNode())->GetAtt().byTransSkinIndex)
								{
									CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("CantMeiRongInTrans")]);
									break;
								}
								DresserFrame *pFrame = (DresserFrame*)TObjRef<GameFrameMgr>()->CreateFrame(_T("World"), _T("Dresser"), _T("DresserFrame"), 0);
								if( P_VALID(pFrame) )
								{
									pFrame->SetNPCID(QuestMgr::Inst()->GetTalkNPCGlobalID());
								}
							}
							Hide();
						}
						break;
					}
				}
			}
		}
		break;

	default:
		break;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// 设置NPC对象名称
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::SetCaption( LPCTSTR szCaption )
{
	m_pCaption->SetText( szCaption );
}

//-----------------------------------------------------------------------------
// 设置对话内容
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::SetContent( LPCTSTR szContent )
{
	tstring strContent = szContent;
	vector<tagIDLink> idLinks;
	vector<tagPosLink> posLinks;
	QuestMgr::Inst()->ParseQuestView(strContent,idLinks,posLinks);
	m_pStcCondition->SetText(strContent.c_str());

	tagPoint size = m_pStcCondition->GetSize();
	if( m_pStcCondition->GetTextTotalHeight() > size.y )
	{
		size.y = m_pStcCondition->GetTextTotalHeight();
		m_pStcCondition->SetResizable(TRUE);
		m_pStcCondition->Resize( size );
		m_pStcCondition->SetResizable(FALSE);
	}
}

//-----------------------------------------------------------------------------
// 清除所有对话选项
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::ClearOptions( void )
{
	if( P_VALID(m_pWnd) )
	{
		m_pListOptions->Clear();
		m_pListOptions->SetCurSelectedRow(0);
		m_curRow = 0;
		m_optAttrs.clear();
	}
}

//-----------------------------------------------------------------------------
// 增加对话选项
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::AddTalkOption( LPCTSTR szOption, DWORD param1, DWORD param2, EOptionType optionType )
{
	m_pListOptions->SetText(m_curRow++, 0, szOption);

	tagOptAttr optAttr;
	optAttr.id = param1;
	optAttr.step = param2;
	optAttr.type = optionType;
	m_optAttrs.push_back(optAttr);
}

//-----------------------------------------------------------------------------
// 设置某NPC的可接任务选项
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::AddAcceptOptions( DWORD npcID )
{
	QuestQuery *pQQ = QuestMgr::Inst()->GetQuery();
	if( !P_VALID(pQQ) )
		return;

	QuestData *pQD = QuestMgr::Inst()->GetData();
	if( !P_VALID(pQD) )
		return;

	vector<UINT16> quests;
	pQQ->GetNPCAcceptQuests(npcID, quests);
	for( size_t i = 0; i < quests.size(); i++ )
	{
		UINT16 questID = quests[i];

		if (OverSeaData::Inst()->IsQuestHide(questID))
			continue;

		tstring name;
		pQD->GetQuestName(questID, name);
		name += g_StrTable[_T("NPCTalk_AcceptOption")];
		m_pListOptions->SetText(m_curRow++, 0, name.c_str());

		tagOptAttr optAttr;
		optAttr.id = questID;
		optAttr.step = 0;
		optAttr.type = EOT_AcceptQuest;
		m_optAttrs.push_back(optAttr);
	}
}

//-----------------------------------------------------------------------------
// 设置某NPC的可交任务选项
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::AddCompleteOptions( DWORD npcID )
{
	QuestQuery *pQQ = QuestMgr::Inst()->GetQuery();
	if( !P_VALID(pQQ) )
		return;

	QuestData *pQD = QuestMgr::Inst()->GetData();
	if( !P_VALID(pQD) )
		return;

	vector<UINT16> quests;

	// 可更新任务
	pQQ->GetNPCUpdateQuests(npcID, quests);
	for( size_t i = 0; i < quests.size(); i++ )
	{
		UINT16 questID = quests[i];
		tstring name;
		pQD->GetQuestName(questID, name);
		name += g_StrTable[_T("NPCTalk_CompleteOption")];
		m_pListOptions->SetText(m_curRow++, 0, name.c_str());

		tagOptAttr optAttr;
		optAttr.id = questID;
		optAttr.step = 0;
		optAttr.type = EOT_CompleteQuest;
		m_optAttrs.push_back(optAttr);
	}

	// 可完成任务
	pQQ->GetNPCCompleteQuests(npcID, quests);
	for( size_t i = 0; i < quests.size(); i++ )
	{
		UINT16 questID = quests[i];
		tstring name;
		pQD->GetQuestName(questID, name);
		name += g_StrTable[_T("NPCTalk_CompleteOption")];
		m_pListOptions->SetText(m_curRow++, 0, name.c_str());

		tagOptAttr optAttr;
		optAttr.id = questID;
		optAttr.step = 0;
		optAttr.type = EOT_CompleteQuest;
		m_optAttrs.push_back(optAttr);
	}

	// 可通过特殊道具完成的任务,屏蔽掉，改成脑残方法了
	/*
	pQQ->GetNPCCompleteQuestsBySpecItem(npcID, quests);
	for (size_t i = 0; i < quests.size(); ++i)
	{
		UINT16 questID = quests[i];
		tstring name;
		pQD->GetQuestName(questID, name);
		name += g_StrTable[_T("NPCTalk_CompleteBySpecItemOption")];
		m_pListOptions->SetText(m_curRow++, 0, name.c_str());

		tagOptAttr optAttr;
		optAttr.id = questID;
		optAttr.step = 0;
		optAttr.type = EOT_CompleteQuestBySpecItem;
		m_optAttrs.push_back(optAttr);
	}
	*/

}

//-----------------------------------------------------------------------------
// 显示对话窗口
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::Show( void )
{
	if( P_VALID(m_pWnd) ) 
	{
		m_pWnd->SetInvisible(FALSE);
		m_pWnd->FlipToTop();

		GUIWnd* pQuizWnd = m_pGUI->GetWnd(_T("\\desktop\\qawin"));
		if(P_VALID(pQuizWnd) && !pQuizWnd->IsInvisible())
			pQuizWnd->FlipToTop();

		// 对话NPC面向玩家
		NPC *pNPC = (NPC*)RoleMgr::Inst()->FindRole(QuestMgr::Inst()->GetTalkNPCGlobalID());
		if( P_VALID(pNPC) && !(RoleMgr::Inst()->IsOutValidDist( QuestMgr::Inst()->GetTalkNPCGlobalID() )) )
		{
			Vector3 localPlayerHead;
			RoleMgr::Inst()->GetLocalPlayer()->GetHeadPos(localPlayerHead);
			pNPC->FaceTo(localPlayerHead);
		}
	}
}

//-----------------------------------------------------------------------------
// 隐藏对话窗口
//-----------------------------------------------------------------------------
VOID NPCTalkFrame::Hide( void )
{
	if( P_VALID(m_pWnd) ) 
	{
		m_pWnd->SetInvisible(TRUE);
        m_isUsingTransportItem  = false;

		// 对话NPC恢复原先朝向
		NPC *pNPC = (NPC*)RoleMgr::Inst()->FindRole(QuestMgr::Inst()->GetTalkNPCGlobalID());
		if( P_VALID(pNPC) )
		{
			pNPC->FaceBack();
		}
	}
}

VOID NPCTalkFrame::Update()
{
    if (m_isUsingTransportItem)
        return;

	m_escapeTime += Kernel::Inst()->GetDeltaTimeDW();
	if( m_escapeTime > 1000 )//每隔1秒检测距离目标NPC的距离
	{
		m_escapeTime = 0;

		if (RoleMgr::Inst()->IsOutValidDist( QuestMgr::Inst()->GetTalkNPCGlobalID() ) )
		{
			Hide();
			m_pMgr->AddToDestroyList(this);
			RoleMgr::Inst()->DestroyVoteUI();

			DeleteAllNPCTalkBox();
		}
	}

	GameFrame::Update();
}

BOOL NPCTalkFrame::EscCancel()
{
	if( P_VALID(m_pWnd) && !m_pWnd->IsInvisible() )
	{
		Hide();
		m_pMgr->AddToDestroyList(this);

		return TRUE;
	}

	return GameFrame::EscCancel();
}

INT NPCTalkFrame::GetTaskNum()
{
	INT nNum = 0;
	for( size_t i = 0; i < m_optAttrs.size(); i++ )
	{
		if( m_optAttrs[i].type == EOT_AcceptQuest || m_optAttrs[i].type == EOT_CompleteQuest )
			nNum++;
	}
	return nNum;
}

VOID NPCTalkFrame::DeleteAllNPCTalkBox()
{
	//创建帮派输入帮派名字的对话框
	if(P_VALID(GetObj("CreateGuild")))
		KillObj("CreateGuild");
}

