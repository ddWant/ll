#include "stdafx.h"
#include "DakFrame.h"
#include "DakData.h"
#include "CombatSysUtil.h"
#include "CurrencyMgr.h"
#include "..\WorldDefine\currency_define.h"
#include "RoleMgr.h"
#include "LocalPlayer.h"
#include "MapMgr.h"
#include "IconStatic.h"
#include "RoleMgr.h"
#include "NPC.h"
#include "ItemMgr.h"

DakFrame::DakFrame( void )
: m_Trunk(this)
, m_pWnd(0)
, m_pCaption(0)
, m_pStcCondition(0)
, m_pListOptions(0)
, m_pBtnCloseWnd1(0)
, m_pBtnCloseWnd2(0)
, m_dwNpcID(GT_INVALID)
, m_dwCurRow(0)
, m_dwTotalRow(0)
, m_escapeTime(0)
{

}

DakFrame::~DakFrame( void )
{
	
}

BOOL DakFrame::Init( GameFrameMgr* pMgr, GameFrame* pFather, DWORD dwParam )
{
	if( !GameFrame::Init(pMgr, pFather, dwParam) )
		return FALSE;

	//--ע��������Ϣ��������
	m_pCmdMgr->Register("NS_Dak",	(NETMSGPROC)m_Trunk.sfp2(&DakFrame::NetRecvNS_Dak), _T("NS_Dak"));

	m_pMgr->RegisterEventHandle(_T("AffirmUseDak_Event"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&DakFrame::OnUseDak));


	return TRUE;
}

BOOL DakFrame::Destroy()
{
	if( P_VALID(m_pWnd) )
		m_pGUI->AddToDestroyList(m_pWnd);

	m_pCmdMgr->UnRegister("NS_Dak",	(NETMSGPROC)m_Trunk.sfp2(&DakFrame::NetRecvNS_Dak));

	m_pMgr->UnRegisterEventHandler(_T("AffirmUseDak_Event"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&DakFrame::OnUseDak));

	if( P_VALID(GetObj("DakTransmitFail_MsgBox")) )
		KillObj("DakTransmitFail_MsgBox");
	if( P_VALID(GetObj("AffirmUseDak_MsgBox")) )
		KillObj("AffirmUseDak_MsgBox");

	return GameFrame::Destroy();;
}

BOOL DakFrame::EscCancel()
{
	m_pFrameMgr->AddToDestroyList(this);
	return TRUE;
}

BOOL DakFrame::ReloadUI()
{
	if( P_VALID(m_pWnd) )
		m_pGUI->AddToDestroyList(m_pWnd);

	tstring strPath = vEngine::g_strLocalPath + _T("\\ui\\npctalk.xml");
	XmlElement element;
	m_pGUI->LoadXml(&element, "VFS_System", strPath.c_str());
	m_pGUI->ChangeXml(&element, "nt_win\\nt_pic\\nt_pic2\\npctalkpic\\contentWnd\\npctalk", "ClassName", "IconStatic");
	m_pGUI->ChangeXml(&element, "nt_win\\nt_pic\\nt_pic2\\npctalkpic\\contentWnd\\npctalk", "PicRect_left", "0");
	m_pGUI->ChangeXml(&element, "nt_win\\nt_pic\\nt_pic2\\npctalkpic\\contentWnd\\npctalk", "PicRect_top", "0");
	m_pGUI->ChangeXml(&element, "nt_win\\nt_pic\\nt_pic2\\npctalkpic\\contentWnd\\npctalk", "PicRect_right", "0");
	m_pGUI->ChangeXml(&element, "nt_win\\nt_pic\\nt_pic2\\npctalkpic\\contentWnd\\npctalk", "PicRect_bottom", "0");
	m_pWnd = m_pGUI->CreateWnd(_T("\\desktop"), &element);

	// ע���¼���������
	m_pGUI->RegisterEventHandler(m_pWnd->GetFullName().c_str(), m_Trunk.sfp1(&DakFrame::EventHandler));

	m_pCaption		= (GUIPatch*)	m_pWnd->GetChild(_T("nt_pic"));
	m_pStcCondition	= (IconStatic*)m_pWnd->GetChild(_T("nt_pic\\nt_pic2\\npctalkpic\\contentWnd\\npctalk"));
	/*tstring strMapName = MapMgr::Inst()->GetCurMapName();*/
	//TCHAR szName[X_LONG_NAME] = {0};
	//_sntprintf( szName, X_LONG_NAME, _T("data\\local\\zhCN\\map\\%s\\%s_delivery.dds"), strMapName.c_str(), strMapName.c_str());
	//m_pStcCondition->SetPic(szName);
	m_pListOptions	= (GUIListBox*)	m_pWnd->GetChild(_T("nt_pic\\nt_pic2\\optpic\\optlist"));
	m_pBtnCloseWnd1	= (GUIButton*)	m_pWnd->GetChild(_T("nt_pic\\new0"));
	m_pBtnCloseWnd2	= (GUIButton*)	m_pWnd->GetChild(_T("nt_pic\\new"));

	m_pListOptions->SetColNum(2, 20);
	m_pListOptions->SetColWidth(0,170);
	m_pListOptions->SetColWidth(1,170);

	m_pWnd->SetInvisible( TRUE );

	return TRUE;
}

DWORD DakFrame::EventHandler( tagGUIEvent* pEvent )
{
	GUIWnd* pWnd = m_pGUI->GetWnd(pEvent->dwWndID);

	switch( pEvent->eEvent )
	{
	case EGUIE_Click:
		{
			if( pWnd == m_pBtnCloseWnd1 || pWnd == m_pBtnCloseWnd2 )//�رմ���
				m_pFrameMgr->AddToDestroyList(this);
		}
		break;
	case EGUIE_ItemClick:
		{
			if(pWnd == m_pListOptions)
			{
				if(pEvent->dwParam1 < m_dwTotalRow)
				{
					m_dwCurRow  = pEvent->dwParam1;

					const tagDakProtoEx* pDakProto = DakData::Inst()->FindDakData(m_dwFunctionID);
					if( !P_VALID(pDakProto) )
						return FALSE;
					switch(pDakProto->dakSite[m_dwCurRow].eCostType)
					{
					case ECCT_BagSilver:
						{
							INT64 n64Silver = CurrencyMgr::Inst()->GetBagSilver();
							if( n64Silver <  pDakProto->dakSite[m_dwCurRow].nCostVal)
							{
								if( P_VALID(GetObj("DakTransmitFail_MsgBox")) )
									KillObj("DakTransmitFail_MsgBox");
								CreateObj("DakTransmitFail_MsgBox", "MsgBox");

								TObjRef<MsgBox>("DakTransmitFail_MsgBox")->Init(_T(""), g_StrTable[_T("Dak_InvalidSilver")], 
									_T("DakTransmitFail_Event"), MsgBoxFlag(MBF_OK), TRUE);
								return FALSE;
							}
						}
						break;
					case ECCT_BagYuanBao:
						{
							INT nYuanBao = CurrencyMgr::Inst()->GetBagYuanbao();
							if( nYuanBao < pDakProto->dakSite[m_dwCurRow].nCostVal )
							{
								if( P_VALID(GetObj("DakTransmitFail_MsgBox")) )
									KillObj("DakTransmitFail_MsgBox");
								CreateObj("DakTransmitFail_MsgBox", "MsgBox");

								TObjRef<MsgBox>("DakTransmitFail_MsgBox")->Init(_T(""), g_StrTable[_T("Dak_InvalidYuanBao")], 
									_T("DakTransmitFail_Event"), MsgBoxFlag(MBF_OK), TRUE);
								return FALSE;
							}
						}
						break;
					}

					if( P_VALID(GetObj("AffirmUseDak_MsgBox")) )
						KillObj("AffirmUseDak_MsgBox");
					CreateObj("AffirmUseDak_MsgBox", "MsgBox");

					TCHAR szBuff[X_LONG_NAME] = {0};
					if (pDakProto->dakSite[m_dwCurRow].eCostType == ECCT_TransferStone)
					{
						_sntprintf( szBuff, X_LONG_NAME, g_StrTable[_T("Dak_UseDak2")], pDakProto->dakSite[m_dwCurRow].strDesc.c_str() );
						TObjRef<MsgBox>("AffirmUseDak_MsgBox")->Init(_T(""), szBuff, 
							_T("AffirmUseDak_Event"), MsgBoxFlag(MBF_OK|MBF_Cancel), TRUE);
					}
					else
					{
						_sntprintf( szBuff, X_LONG_NAME, g_StrTable[_T("Dak_UseDak")], pDakProto->dakSite[m_dwCurRow].strDesc.c_str() );
						TObjRef<MsgBox>("AffirmUseDak_MsgBox")->Init(_T(""), szBuff, 
							_T("AffirmUseDak_Event"), MsgBoxFlag(MBF_OK|MBF_Cancel), TRUE);
					}
					

					m_pWnd->SetRefresh(TRUE);
				}
			}
		}
		break;
	default:
		break;
	}
	return TRUE;
}

void DakFrame::ShowWnd()
{
	if( P_VALID(m_pWnd) )
		m_pWnd->SetInvisible( FALSE );
}

void DakFrame::SetDakData(DWORD id, DWORD dwFunctionID)
{ 
	m_dwNpcID = id; 
	m_dwFunctionID = dwFunctionID;

	TCHAR szName[X_LONG_NAME] = {0};
	tstring strMapName = MapMgr::Inst()->GetCurMapName();

	NPC* pNpc = static_cast<NPC*>(RoleMgr::Inst()->FindRole(m_dwNpcID));
	if (!P_VALID(pNpc))
		return;

	tstring qiankun = _T("CSS");
	tstring modelName = pNpc->GetProto()->szModelName;

	if ( modelName == qiankun )//Ǭ��ʯ
	{
		_sntprintf( szName, X_LONG_NAME, _T("data\\local\\zhCN\\map\\%s\\%s_delivery.dds"), strMapName.c_str(), strMapName.c_str());
	}
	else //��վ
	{
		_sntprintf( szName, X_LONG_NAME, _T("data\\local\\zhCN\\map\\%s\\%s_station.dds"), strMapName.c_str(), strMapName.c_str());

	}
	m_pStcCondition->SetPic(szName);
	

	const tagDakProtoEx* pDakProto = DakData::Inst()->FindDakData(m_dwFunctionID);
	if( !P_VALID(pDakProto) )
		return;
	for(int i=0; i<MAX_DAK_SITE_NUM; i++)
	{
		if(pDakProto->dakSite[i].dwMapID!=0
			&& pDakProto->dakSite[i].dwMapID!=GT_INVALID)
		{
			tstringstream ss;
			tstringstream tt;
			ss << pDakProto->dakSite[i].strDesc;
			switch(pDakProto->dakSite[i].eCostType)
			{
			case ECCT_Null:
			case ECCT_BagSilver:
				{
					if(pDakProto->dakSite[i].nCostVal <= 0)
						tt << g_StrTable[_T("Dak_Free")];
					else if(pDakProto->dakSite[i].nCostVal<10000)
						tt <<pDakProto->dakSite[i].nCostVal << g_StrTable[_T("Dak_Silver")];
					else
					{
						DWORD dwGloden = MSilver2DBGold(pDakProto->dakSite[i].nCostVal);
						DWORD dwSilver = MSilver2DBSilver(pDakProto->dakSite[i].nCostVal);
						tt << dwGloden << g_StrTable[_T("Dak_Golden")] << dwSilver << g_StrTable[_T("Dak_Silver")];
					}
				}
				break;
			case ECCT_BagYuanBao:
				tt <<pDakProto->dakSite[i].nCostVal << g_StrTable[_T("Dak_YuanBao")];
				break;
			case ECCT_GuildContribe:
				tt <<pDakProto->dakSite[i].nCostVal << g_StrTable[_T("Dak_GuildContribe")];
				break;
			case ECCT_GuildExploit:
				tt <<pDakProto->dakSite[i].nCostVal << g_StrTable[_T("Dak_GuildExploit")];
				break;
			}
			m_pListOptions->SetText(m_dwTotalRow, 0, ss.str().c_str());
			m_pListOptions->SetText(m_dwTotalRow, 1, tt.str().c_str());
			m_dwTotalRow++;
		}
	}
}

VOID DakFrame::SetCaption( LPCTSTR szCaption )
{
	m_pCaption->SetText( szCaption );
}

DWORD DakFrame::NetRecvNS_Dak( tagNS_Dak* pMsg, DWORD dwParam )
{

	switch(pMsg->dwErrorCode)
	{
	case E_Success:
		m_pFrameMgr->AddToDestroyList(this);
		break;
	case E_Dak_NPCNotFound:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_NPCNotFound")]);
		break;
	case E_Dak_NPCNotValid:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_NPCNotValid")]);
		break;
	case E_Dak_TooFar:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_ToFar")]);
		break;
	case E_Dak_NotExist:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_NotExist")]);
		break;
	case E_Dak_TargetMap_Invalid:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_TargetMap_Invalid")]);
		break;
	case E_Dak_NotEnough_Money:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_NotEnough_Money")]);
		break;
	case E_Dak_ChangeMap_Failed:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_ChangeMap_Failed")]);
		break;
	case E_YuanBao_NotEnough:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_InvalidYuanBao")]);
		break;
	case E_Role_CantMove:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_CannotMove")]);
		break;
	case E_Dak_NoStone:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Dak_NoStone")]);
		break;
	default:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("SystemError")]);
		break;
	}
	
	return 0;
}	

VOID DakFrame::Update()
{
	m_escapeTime += Kernel::Inst()->GetDeltaTimeDW();
	if( m_escapeTime > 1000 )//ÿ��1�������Ŀ��NPC�ľ���
	{
		m_escapeTime = 0;
		LocalPlayer* pLocalPlayer = RoleMgr::Inst()->GetLocalPlayer();
		if( !P_VALID(pLocalPlayer) )
		{
			Destroy();
			return;
		}
		if (RoleMgr::Inst()->IsOutValidDist( m_dwNpcID ) )
		{
			m_pMgr->AddToDestroyList(this);
		}
	}
}

DWORD DakFrame::OnUseDak( tagMsgBoxEvent* pGameEvent )
{
	if( MBF_OK == pGameEvent->eResult )	
	{
		const tagDakProtoEx* pDakProto = DakData::Inst()->FindDakData(m_dwFunctionID);
		if( !P_VALID(pDakProto) )
			return 0;

		if (pDakProto->dakSite[m_dwCurRow].eCostType == ECCT_TransferStone)
		{
			Item* pItem = ItemMgr::Inst()->GetFirstSpecFuncItemFromPocket( EISF_TransferStone );
			if (!P_VALID(pItem))
			{
				CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("TransferFailedNoStone")]);
				return 0;
			}
		}
		
		tagNC_Dak dakMsg;
		dakMsg.dwMapID = pDakProto->dakSite[m_dwCurRow].dwMapID;
		dakMsg.dwNPCID = m_dwNpcID;
		dakMsg.nIndex = m_dwCurRow;
		m_pSession->Send(&dakMsg);
		m_pMgr->AddToDestroyList(this);
	}

	return 0;
}