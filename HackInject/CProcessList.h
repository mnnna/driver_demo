﻿#pragma once


// CProcessList 对话框

class CProcessList : public CDialogEx
{
	DECLARE_DYNAMIC(CProcessList)

public:
	CProcessList(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CProcessList();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListCtrl m_ProcessList;
	VOID InitProcessList();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedFlush();

};
