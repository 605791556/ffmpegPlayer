
// ffmpegTest.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CffmpegTestApp:
// �йش����ʵ�֣������ ffmpegTest.cpp
//

class CffmpegTestApp : public CWinApp
{
public:
	CffmpegTestApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CffmpegTestApp theApp;