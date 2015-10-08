//////////////////////////////////////////////////////////////////////////
//  Class Name: SItemPanel
//     Creator: Huang Jianxiong
//     Version: 2011.10.20 - 1.0 - Create
//////////////////////////////////////////////////////////////////////////

#pragma  once
#include "souistd.h"
#include "core/SItempanel.h"

#pragma warning(disable:4018)

namespace SOUI
{


SItemPanel::SItemPanel(SWindow *pFrameHost,pugi::xml_node xmlNode,IItemContainer *pItemContainer)
    :m_pFrmHost(pFrameHost)
    ,m_pItemContainer(pItemContainer)
    ,m_dwData(0)
    ,m_crBk(CR_INVALID)
    ,m_crHover(CR_INVALID)
    ,m_crSelBk(RGBA(0,0,128,255))
    ,m_lpItemIndex(-1)
{
    SASSERT(m_pFrmHost);
    SASSERT(m_pItemContainer);
    SetContainer(this);
    if(xmlNode) 
    {
        InitFromXml(xmlNode);
        BuildWndTreeZorder();
    }
    m_evtSet.addEvent(EVENTID(EventItemPanelClick));
    m_evtSet.addEvent(EVENTID(EventItemPanelDbclick));
    m_evtSet.addEvent(EVENTID(EventItemPanelRclick));
    m_evtSet.addEvent(EVENTID(EventItemPanelHover));
    m_evtSet.addEvent(EVENTID(EventItemPanelLeave));
}

void SItemPanel::OnFinalRelease()
{
    AddRef();//防止重复进入该函数
    SSendMessage(WM_DESTROY);
    __super::OnFinalRelease();
}

LRESULT SItemPanel::DoFrameEvent(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    AddRef();

    if(!IsDisabled())
    {
        switch(uMsg)
        {
        case WM_MOUSEHOVER: 
            {
                ModifyState(WndState_Hover,0,TRUE);
                EventItemPanelHover evt(this);
                FireEvent(evt);
                break;
            }
        case WM_MOUSELEAVE: 
            {
                ModifyState(0,WndState_Hover,TRUE);
                EventItemPanelLeave evt(this);
                FireEvent(evt);
                break;
            }
        case WM_LBUTTONDOWN:
            {
                EventItemPanelClick evt(this);
                FireEvent(evt);
                break;
            }
        case WM_RBUTTONDOWN:
            {
                EventItemPanelRclick evt(this);
                FireEvent(evt);
                break;
            }
        case WM_LBUTTONDBLCLK:
            {
                EventItemPanelDbclick evt(this);
                FireEvent(evt);
                break;
            }
        }
    }

    SetMsgHandled(FALSE);
    LRESULT lRet=__super::DoFrameEvent(uMsg,wParam,lParam);
    Release();
    return lRet;
}

BOOL SItemPanel::OnFireEvent(EventArgs &evt)
{
    EventOfPanel evt2(m_pFrmHost,this,&evt);
    return m_pFrmHost->FireEvent(evt2);
}

CRect SItemPanel::GetContainerRect()
{
    CRect rcItem;
    m_pItemContainer->OnItemGetRect(this,rcItem);
    return rcItem;
}

IRenderTarget * SItemPanel::OnGetRenderTarget(const CRect & rc,DWORD gdcFlags)
{
    CRect rcItem=GetItemRect();
    CRect rcInvalid=rc;
    rcInvalid.OffsetRect(rcItem.TopLeft());
    IRenderTarget *pRT=m_pFrmHost->GetRenderTarget(rcInvalid,gdcFlags);
    if(gdcFlags == OLEDC_PAINTBKGND)
    {//调用frmhost的GetRenderTarget时，不会绘制frmHost的背景。注意此外只画背景，不画前景,因为itempanel就是前景
        m_pFrmHost->SSendMessage(WM_ERASEBKGND, (WPARAM)pRT);
    }
    pRT->OffsetViewportOrg(rcItem.left,rcItem.top);
    return pRT;
}

void SItemPanel::OnReleaseRenderTarget(IRenderTarget *pRT,const CRect &rc,DWORD gdcFlags)
{
    CRect rcItem=GetItemRect();
    pRT->OffsetViewportOrg(-rcItem.left,-rcItem.top);
    m_pFrmHost->ReleaseRenderTarget(pRT);
}

void SItemPanel::OnRedraw(const CRect &rc)
{
    if(m_pFrmHost->IsUpdateLocked()) return;

    CRect rcItem=GetItemRect();
    if(!rcItem.IsRectNull() && m_pFrmHost->IsVisible(TRUE))
    {
        if(m_pItemContainer->IsItemRedrawDelay())
        {
            CRect rc2(rc);
            rc2.OffsetRect(rcItem.TopLeft());
            rc2.IntersectRect(rc2,rcItem);
            CRect rcHostClient = m_pFrmHost->GetClientRect();
            rc2.IntersectRect(rc2,rcHostClient);
            m_pFrmHost->InvalidateRect(rc2);
        }else
        {
            IRenderTarget *pRT=OnGetRenderTarget(rc,OLEDC_PAINTBKGND);
            CAutoRefPtr<IRegion> rgn;
            GETRENDERFACTORY->CreateRegion(&rgn);
            rgn->CombineRect(&rc,RGN_COPY);
            RedrawRegion(pRT,rgn);
            OnReleaseRenderTarget(pRT,rc,OLEDC_PAINTBKGND);
        }
    }
}

BOOL SItemPanel::OnReleaseSwndCapture()
{
    if(!__super::OnReleaseSwndCapture()) return FALSE;
    m_pItemContainer->OnItemSetCapture(this,FALSE);
    return TRUE;
}

SWND SItemPanel::OnSetSwndCapture(SWND swnd)
{
    m_pItemContainer->OnItemSetCapture(this,TRUE);
    return __super::OnSetSwndCapture(swnd);
}

HWND SItemPanel::GetHostHwnd()
{
    return m_pFrmHost->GetContainer()->GetHostHwnd();
}

const SStringW & SItemPanel::GetTranslatorContext()
{
    return m_pFrmHost->GetContainer()->GetTranslatorContext();
}

BOOL SItemPanel::IsTranslucent() const
{
    return m_pFrmHost->GetContainer()->IsTranslucent();
}

BOOL SItemPanel::IsSendWheel2Hover() const
{
    return m_pFrmHost->GetContainer()->IsSendWheel2Hover();
}

BOOL SItemPanel::OnCreateCaret(SWND swnd, HBITMAP hBmp,int nWidth,int nHeight )
{
    return m_pFrmHost->GetContainer()->OnCreateCaret(swnd,hBmp,nWidth,nHeight);
}

BOOL SItemPanel::OnShowCaret( BOOL bShow )
{
    return m_pFrmHost->GetContainer()->OnShowCaret(bShow);
}

BOOL SItemPanel::OnSetCaretPos( int x,int y )
{
    CRect rcItem=GetItemRect();
    x+=rcItem.left,y+=rcItem.top;
    return m_pFrmHost->GetContainer()->OnSetCaretPos(x,y);
}

BOOL SItemPanel::UpdateWindow()
{
    return m_pFrmHost->GetContainer()->UpdateWindow();
}

void SItemPanel::ModifyItemState(DWORD dwStateAdd, DWORD dwStateRemove)
{
    ModifyState(dwStateAdd,dwStateRemove,FALSE);
}

SWND SItemPanel::SwndFromPoint(POINT ptHitTest, BOOL bOnlyText)
{
    SWND hRet=__super::SwndFromPoint(ptHitTest,bOnlyText);
    if(hRet==m_swnd) hRet=NULL;
    return hRet;
}

void SItemPanel::Draw(IRenderTarget *pRT,const CRect & rc)
{
    UpdateLayout();

    m_style.m_crBg=m_crBk;
    if((m_dwState & WndState_Hover) && m_crHover != CR_INVALID) m_style.m_crBg=m_crHover;
    if((m_dwState & WndState_Check) && m_crSelBk != CR_INVALID) m_style.m_crBg=m_crSelBk;

    //不继承宿主的字体，从系统字体开始
    SPainter painter;
    BeforePaint(pRT,painter);

    CAutoRefPtr<IRegion> rgn;
    pRT->GetClipRegion(&rgn);
    
    pRT->OffsetViewportOrg(rc.left,rc.top);
    if(rgn)
    {
        rgn->Offset(-rc.TopLeft());
    }
    BuildWndTreeZorder();
    RedrawRegion(pRT,rgn);
    pRT->OffsetViewportOrg(-rc.left,-rc.top);
    
    AfterPaint(pRT,painter);
}

void SItemPanel::SetSkin(ISkinObj *pSkin)
{
    m_pBgSkin=pSkin;
}

void SItemPanel::SetColor(COLORREF crBk,COLORREF crSelBk)
{
    m_crBk=crBk;
    m_crSelBk=crSelBk;
}

BOOL SItemPanel::NeedRedrawWhenStateChange()
{
    return TRUE;
}

CRect SItemPanel::GetItemRect()
{
    CRect rcItem;
    m_pItemContainer->OnItemGetRect(this,rcItem);
    return rcItem;
}
void SItemPanel::SetItemCapture(BOOL bCapture)
{
    m_pItemContainer->OnItemSetCapture(this,bCapture);
}

void SItemPanel::SetItemData(LPARAM dwData)
{
    m_dwData=dwData;
}

LPARAM SItemPanel::GetItemData()
{
    return m_dwData;
}

BOOL SItemPanel::OnUpdateToolTip(CPoint pt, SwndToolTipInfo &tipInfo)
{
    CRect rcItem=GetItemRect();
    if(m_hHover==m_swnd)
    {
        tipInfo.swnd = m_swnd;
        tipInfo.dwCookie =0;
        tipInfo.rcTarget = rcItem;
        tipInfo.strTip = m_strToolTipText;
        return TRUE;
    }
    
    SWindow *pHover=SWindowMgr::GetWindow(m_hHover);
    if(!pHover || pHover->IsDisabled(TRUE))
    {
        tipInfo.swnd=0;
        return TRUE;
    }
    
    pt -= rcItem.TopLeft();
    BOOL bRet=pHover->OnUpdateToolTip(pt,tipInfo);
    if(bRet)
    {
        tipInfo.rcTarget.OffsetRect(rcItem.TopLeft());
    }
    return bRet;
}

BOOL SItemPanel::RegisterTimelineHandler( ITimelineHandler *pHandler )
{
    BOOL bRet=SwndContainerImpl::RegisterTimelineHandler(pHandler);
    if(bRet && m_lstTimelineHandler.GetCount()==1) m_pFrmHost->GetContainer()->RegisterTimelineHandler(this);
    return bRet;
}

BOOL SItemPanel::UnregisterTimelineHandler( ITimelineHandler *pHandler )
{
    BOOL bRet=SwndContainerImpl::UnregisterTimelineHandler(pHandler);
    if(bRet && m_lstTimelineHandler.IsEmpty()) m_pFrmHost->GetContainer()->UnregisterTimelineHandler(this);
    return bRet;
}

SMessageLoop * SItemPanel::GetMsgLoop()
{
    return m_pFrmHost->GetContainer()->GetMsgLoop();
}

IScriptModule * SItemPanel::GetScriptModule()
{
    return m_pFrmHost->GetContainer()->GetScriptModule();
}

void SItemPanel::FrameToHost(RECT & rc)
{
    CRect rcItem = GetItemRect();
    ::OffsetRect(&rc,rcItem.left,rcItem.top);
}

void SItemPanel::RequestRelayout(SWindow *pSource /*=NULL */)
{
    m_bLayoutDirty=TRUE;
    m_pItemContainer->OnItemRequestRelayout(this);
}

SItemPanel * SItemPanel::Create(SWindow *pFrameHost,pugi::xml_node xmlNode,IItemContainer *pItemContainer)
{
    return new SItemPanel(pFrameHost,xmlNode,pItemContainer);
}

void SItemPanel::BeforePaint(IRenderTarget *pRT, SPainter &painter)
{
    pRT->SelectObject(SFontPool::getSingleton().GetFont(FF_DEFAULTFONT));
    pRT->SetTextColor(RGBA(0,0,0,255));
}

}//namespace SOUI