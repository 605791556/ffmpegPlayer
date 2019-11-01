
// ffmpegTestDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ffmpegTest.h"
#include "ffmpegTestDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CffmpegTestDlg �Ի���


DWORD WINAPI DecodeThread(LPVOID lpParam);
DWORD WINAPI VideoThread(LPVOID lpParam);
DWORD WINAPI AudioThread(LPVOID lpParam);

CffmpegTestDlg::CffmpegTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CffmpegTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pFormatCtx = nullptr;
	m_pvCodecCtxOrg = nullptr;
	m_pvCodecCtx = nullptr;
	m_pvCodec = nullptr;
	m_videoStream = -1;
	m_paCodecCtxOrg = nullptr;
	m_paCodecCtx = nullptr;
	m_paCodec = nullptr;

	m_sdlWindow = nullptr;
	m_sdlRenderer = nullptr;
	m_sdlTexture = nullptr;

	m_AudioStream = -1;
	m_hSupendEvent = CreateEvent(NULL, TRUE, TRUE, NULL); //��ʼ���ź�
	m_hDecodeEvent = CreateEvent(NULL, TRUE, TRUE, NULL); //��ʼ���ź�
	m_hVideoEvent = CreateEvent(NULL, TRUE, TRUE, NULL); //��ʼ���ź�
	m_hAudioEvent = CreateEvent(NULL, TRUE, TRUE, NULL); //��ʼ���ź�
	m_hACallEvent = CreateEvent(NULL, TRUE, TRUE, NULL); //��ʼ���ź�
	m_hSupendEvent = CreateEvent(NULL, TRUE, TRUE, NULL); //��ʼ���ź�
	m_hDecodeHandel = INVALID_HANDLE_VALUE;
	m_hVPlayHandel = INVALID_HANDLE_VALUE;
	m_hAPlayHandel = INVALID_HANDLE_VALUE;
}

CffmpegTestDlg::~CffmpegTestDlg()
{

}

void CffmpegTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_FILE, m_editFile);
	DDX_Control(pDX, IDC_PIC, m_pic);
	DDX_Control(pDX, IDC_SLIDER, m_slider);
	DDX_Control(pDX, IDC_GROUP, m_group);
}

BEGIN_MESSAGE_MAP(CffmpegTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CffmpegTestDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_OPEN, &CffmpegTestDlg::OnBnClickedBtnOpen)
	ON_BN_CLICKED(IDC_BTN_DECODE_ONE, &CffmpegTestDlg::OnBnClickedPlay)
	ON_MESSAGE(WM_SHOW_BK, &CffmpegTestDlg::OnShowBk)
	ON_MESSAGE(WM_UPDATE_SLIDER, &CffmpegTestDlg::OnUpdateSlider)
	ON_MESSAGE(WM_ERROR_MSG, &CffmpegTestDlg::OnErrorMsg)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BTN_PAUSE, &CffmpegTestDlg::OnBnClickedBtnPause)
	ON_BN_CLICKED(IDC_BTN_STOP, &CffmpegTestDlg::OnBnClickedBtnStop)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER, &CffmpegTestDlg::OnNMReleasedcaptureSlider)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CffmpegTestDlg ��Ϣ�������

BOOL CffmpegTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	av_register_all(); // ע��֧�ֵ��ļ���ʽ����Ӧ��codec
	SetPlayType(EM_PLAY_TYPE_STOP);
	PostMessage(WM_SHOW_BK, NULL, 0);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		MessageBox(L"sdl init error");
	}

	//��ʾ��MFC�ؼ���
	m_sdlWindow = SDL_CreateWindowFrom(GetDlgItem(IDC_PIC)->GetSafeHwnd());
	if (!m_sdlWindow)
	{
		MessageBox(L"SDL_CreateWindowFrom error");
	}

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CffmpegTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CffmpegTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CffmpegTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CffmpegTestDlg::SetPlayType(EM_PLAY_TYPE type)
{
	m_playType = type;
	switch (type)
	{
	case EM_PLAY_TYPE_PLAY:
	{
		GetDlgItem(IDC_BTN_OPEN)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_DECODE_ONE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_PAUSE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_STOP)->EnableWindow(TRUE);
		SetEvent(m_hSupendEvent);
		break;
	}
	case EM_PLAY_TYPE_PAUSE:
	{
		GetDlgItem(IDC_BTN_DECODE_ONE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_PAUSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_STOP)->EnableWindow(TRUE);
		ResetEvent(m_hSupendEvent);
		break;
	}
	case EM_PLAY_TYPE_STOP:
	{
		SetEvent(m_hSupendEvent);
		if (m_hDecodeHandel != INVALID_HANDLE_VALUE)
		{
			WaitForSingleObject(m_hDecodeHandel, INFINITE);
			m_hDecodeHandel = INVALID_HANDLE_VALUE;
		}
		if (m_hVPlayHandel != INVALID_HANDLE_VALUE)
		{
			WaitForSingleObject(m_hVPlayHandel, INFINITE);
			m_hVPlayHandel = INVALID_HANDLE_VALUE;
		}
		if (m_hAPlayHandel != INVALID_HANDLE_VALUE)
		{
			WaitForSingleObject(m_hAPlayHandel, INFINITE);
			m_hAPlayHandel = INVALID_HANDLE_VALUE;
		}

		ClearQueue();
		GetDlgItem(IDC_BTN_OPEN)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_DECODE_ONE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_PAUSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);
		break;
	}
	}

}

EM_PLAY_TYPE CffmpegTestDlg::GetPlayType()
{
	return m_playType;
}

void CffmpegTestDlg::ClearQueue()
{
	while (!m_qAbuf.empty())
	{
		ST_ABUF* stBuf = &m_qAbuf.front();
		delete[] stBuf->buf;
		m_qAbuf.pop();
	}
	while (!m_VList.empty())
	{
		AVPacket* pkt = &m_VList.front();
		av_free_packet(pkt);
		m_VList.pop();
	}
	while (!m_AList.empty())
	{
		AVPacket* pkt = &m_AList.front();
		av_free_packet(pkt);
		m_AList.pop_front();
	}
}

void CffmpegTestDlg::OnBnClickedOk()
{

}

void CffmpegTestDlg::showBk(CString strPath)
{
	int width, height;
	CImage  image;
	CRect   rect;

	image.Load(strPath);

	CRect rectControl; //�ؼ����ζ���
	CRect rectPicture;

	int x = image.GetWidth();
	int y = image.GetHeight();
	CWnd  *pWnd = GetDlgItem(IDC_PIC);
	pWnd->GetClientRect(rectControl);

	CDC *pDc = GetDlgItem(IDC_PIC)->GetDC();
	SetStretchBltMode(pDc->m_hDC, STRETCH_HALFTONE);

	rectPicture = CRect(rectControl.TopLeft(), CSize((int)rectControl.Width(), (int)rectControl.Height()));

	((CStatic*)GetDlgItem(IDC_PIC))->SetBitmap(NULL);

	image.Draw(pDc->m_hDC, rectPicture);//��ͼƬ���Ƶ�Picture�ؼ���ʾ�ľ�������

	image.Destroy();
	pWnd->ReleaseDC(pDc);
}

void CffmpegTestDlg::saveFrame(AVFrame* pFrame, int width, int height, int iFrame)
{
	USES_CONVERSION;

	char buf[5] = { 0 };
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	FILE *fp;

	char filename[255] = { 0 };
	//�ļ����·���������Լ����޸�
	sprintf_s(filename, 255, "%d.bmp", iFrame);
	if ((fp = fopen(filename, "wb+")) == NULL)
	{
		MessageBox(L"fopen file failed!", L"");
		return;
	}

	bmpheader.bfType = 0x4d42;
	bmpheader.bfReserved1 = 0;
	bmpheader.bfReserved2 = 0;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize = bmpheader.bfOffBits + width*height * 24 / 8;

	bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.biWidth = width;
	bmpinfo.biHeight = -height;
	bmpinfo.biPlanes = 1;
	bmpinfo.biBitCount = 24;
	bmpinfo.biCompression = BI_RGB;
	bmpinfo.biSizeImage = (width * 24 + 31) / 32 * 4 * height;
	bmpinfo.biXPelsPerMeter = 100;
	bmpinfo.biYPelsPerMeter = 100;
	bmpinfo.biClrUsed = 0;
	bmpinfo.biClrImportant = 0;

	fwrite(&bmpheader, sizeof(bmpheader), 1, fp);
	fwrite(&bmpinfo, sizeof(bmpinfo), 1, fp);
	fwrite(pFrame->data[0], width*height * 24 / 8, 1, fp);

	fclose(fp);
}


HRESULT CffmpegTestDlg::OnShowBk(WPARAM wParam, LPARAM lParam)
{
	showBk();
	return 0;
}

HRESULT CffmpegTestDlg::OnUpdateSlider(WPARAM wParam, LPARAM lParam)
{
	int pos = lParam;
	double all_sec = (double)pos / 1000;

	int sec = (int)all_sec % 60;
	int min = ((int)all_sec / 60) % 60;
	int h = all_sec / 3600;
	int ms = (all_sec - (int)all_sec) * 100;
	CString str;
	str.Format(L"%02d:%02d:%02d:%02d", h, min, sec, ms);
	SetDlgItemTextW(IDC_STA_TIME, str);

	int max = m_slider.GetRangeMax();
	if (max <= pos)
	{
		max = 0;
	}
	m_slider.SetPos(pos);
	return 0;
}

HRESULT CffmpegTestDlg::OnErrorMsg(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case EM_SEND_PACKET_ERROR:
	{
		MessageBox(L"send packet error!", L"����");
		break;
	}
	case EM_AVCODE_OPEN_ERROR:
	{
		MessageBox(L"avcode open error!", L"����");
		break;
	}
	case EM_SDL_OPENAUDIO_ERROR:
	{
		MessageBox(L"sdl open audio error!", L"����");
		break;
	}
	
	}
	
	return 0;
}

void CffmpegTestDlg::FreeVariable()
{
	m_videoStream = -1;
	m_AudioStream = -1;
	if (m_pvCodecCtxOrg)
	{
		avcodec_close(m_pvCodecCtxOrg);
		m_pvCodecCtxOrg = nullptr;
	}
	if (m_pvCodecCtx)
	{
		avcodec_close(m_pvCodecCtx);
		m_pvCodecCtx = nullptr;
	}
	if (m_paCodecCtxOrg)
	{
		avcodec_close(m_paCodecCtxOrg);
		m_paCodecCtxOrg = nullptr;
	}
	if (m_paCodecCtx)
	{
		avcodec_close(m_paCodecCtx);
		m_paCodecCtx = nullptr;
	}
	if (m_pFormatCtx)
	{
		avformat_close_input(&m_pFormatCtx);
		m_pFormatCtx = nullptr;
	}
	if (m_sdlRenderer)
	{
		SDL_DestroyRenderer(m_sdlRenderer);
		m_sdlRenderer = nullptr;
	}
	if (m_sdlTexture)
	{
		SDL_DestroyTexture(m_sdlTexture);
		m_sdlTexture = nullptr;
	}
}

void  SDL_AudioCallPlay(void *udata, Uint8 *stream, int len)
{
	CffmpegTestDlg* pThis = (CffmpegTestDlg*)udata;
	SDL_memset(stream, 0, len);

	std::lock_guard<mutex> mtx_locker(pThis->m_CMutex);
	if (!pThis->m_qAbuf.empty())
	{
		static int send_len = 0;

		ST_ABUF* st_buf = &pThis->m_qAbuf.front();
		int cp_len = (len > st_buf->buf_len ? st_buf->buf_len : len);
		if (cp_len > 0)
		{
			memcpy(stream, st_buf->buf + send_len, cp_len);
			send_len += cp_len;
			st_buf->buf_len -= cp_len;
			if (st_buf->buf_len == 0)
			{
				send_len = 0;
				delete[] st_buf->buf;	
				pThis->m_qAbuf.pop();
			}
		}
		SetEvent(pThis->m_hACallEvent);
		WaitForSingleObject(pThis->m_hSupendEvent, INFINITE);//��ͣ�󣬴˴���һֱ�ȴ���ֱ��SetEvent(m_hSupendEvent)
		ResetEvent(pThis->m_hACallEvent);
	}
}

int GetSampleCount(AVSampleFormat out_sample_fmt)
{
	int count = 0;
	switch (out_sample_fmt)
	{
	case AV_SAMPLE_FMT_S32:count = 4; break;
	case AV_SAMPLE_FMT_S16:count = 2; break;
	case AV_SAMPLE_FMT_U8: count = 1; break;
	}
	return count;
}

/*
*�����̣߳���������AVPacket�������m_VList��m_AList��
*/
DWORD WINAPI DecodeThread(LPVOID lpParam)
{
	CffmpegTestDlg* pThis = (CffmpegTestDlg*)lpParam;

	do
	{
		if (!pThis->m_pvCodecCtxOrg || !pThis->m_pvCodec)
		{
			break;
		}
		if (avcodec_open2(pThis->m_pvCodecCtx, pThis->m_pvCodec, nullptr) < 0)
		{
			pThis->PostMessageW(WM_ERROR_MSG,EM_AVCODE_OPEN_ERROR);
			break;
		}

		//������Ƶ�����߳�
		pThis->m_hVPlayHandel = CreateThread(NULL, NULL, VideoThread, pThis, 0, 0);
		pThis->m_hAPlayHandel = CreateThread(NULL, NULL, AudioThread, pThis, 0, 0);
		
		//����ǰ��seek�����
		av_seek_frame(pThis->m_pFormatCtx, pThis->m_videoStream, 0, AVSEEK_FLAG_BACKWARD);
		
		while (true)
		{
			SetEvent(pThis->m_hDecodeEvent);//��Ϊ��ͣ״̬��ִ�е��˴��ű�ʾ���߳�������ͣ��
			WaitForSingleObject(pThis->m_hSupendEvent, INFINITE);//��ͣ�󣬴˴���һֱ�ȴ���ֱ��SetEvent(m_hSupendEvent)
			ResetEvent(pThis->m_hDecodeEvent);
			if (pThis->GetPlayType() == EM_PLAY_TYPE_STOP)
			{
				break;
			}

			{
				std::lock_guard<mutex> mtx_locker(pThis->m_VMutex);
				int v_size = pThis->m_VList.size();
				if (v_size >= MAX_VQUEUE)
				{
					//�����б���ʱ�ȴ������߳������ټ���������룬�����ڴ�ռ��Խ��Խ��
					continue;
				}
			}
			
			AVPacket packet;
			if (av_read_frame(pThis->m_pFormatCtx, &packet) >= 0)
			{
				pThis->m_bReadPacketEnd = false;
				if (packet.stream_index == pThis->m_videoStream)
				{
					std::lock_guard<mutex> mtx_locker(pThis->m_VMutex);
					pThis->m_VList.push(packet);
				}
				else if (packet.stream_index == pThis->m_AudioStream)
				{	
					std::lock_guard<mutex> mtx_locker(pThis->m_AMutex);
					pThis->m_AList.push_back(packet);
					
				}
			}
			else
				pThis->m_bReadPacketEnd = true; //����ʱ���Ż�δ����������������ǰ������m_bReadPacketEnd = false
		}	
	} while (0);
	return 0;
}

/*
*ͼƬ�����̣߳��Ӷ���m_qVbuf��ȡ��ͼƬ֡����ͨ��SDL�ӿ���ʾ
*/
DWORD WINAPI VideoThread(LPVOID lpParam)
{
	CffmpegTestDlg* pThis = (CffmpegTestDlg*)lpParam;

	//------------SDL----------------
	if (!pThis->m_sdlRenderer && !pThis->m_sdlTexture)
	{
		pThis->m_sdlRenderer = SDL_CreateRenderer(pThis->m_sdlWindow, -1, 0);
		pThis->m_sdlTexture = SDL_CreateTexture(pThis->m_sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pThis->m_pvCodecCtx->width, pThis->m_pvCodecCtx->height);
	}
	//----------------------------------

	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameYUV420 = av_frame_alloc();

	// ʹ�õĻ������Ĵ�С
	int numBytes = 0;
	uint8_t* buffer = nullptr;

	//AV_PIX_FMT_BGR24 / AV_PIX_FMT_YUV420P
	numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pThis->m_pvCodecCtx->width, pThis->m_pvCodecCtx->height);
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	avpicture_fill((AVPicture*)pFrameYUV420, buffer, AV_PIX_FMT_YUV420P, pThis->m_pvCodecCtx->width, pThis->m_pvCodecCtx->height);

	struct SwsContext* sws_ctx = nullptr;
	sws_ctx = sws_getContext(pThis->m_pvCodecCtx->width, pThis->m_pvCodecCtx->height, pThis->m_pvCodecCtx->pix_fmt,
		pThis->m_pvCodecCtx->width, pThis->m_pvCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

	while (true)
	{
		SetEvent(pThis->m_hVideoEvent);//��Ϊ��ͣ״̬��ִ�е��˴��ű�ʾ������ͣ��
		WaitForSingleObject(pThis->m_hSupendEvent, INFINITE);//��ͣ�󣬴˴���һֱ�ȴ���ֱ��SetEvent(m_hSupendEvent)
		ResetEvent(pThis->m_hVideoEvent);
		if (pThis->GetPlayType() == EM_PLAY_TYPE_STOP)
		{
			break;
		}

		if (!pThis->m_VList.empty())
		{
			AVPacket* packet = &pThis->m_VList.front();
			if (packet->buf == NULL)
			{
				av_free_packet(packet);
				std::lock_guard<mutex> mtx_locker(pThis->m_VMutex);
				pThis->m_VList.pop();
				continue;
			}
			//��Ƶ����
			int ret = avcodec_send_packet(pThis->m_pvCodecCtx, packet);
			if (ret < 0)
			{
				av_free_packet(packet);
				pThis->PostMessage(WM_ERROR_MSG, NULL, EM_SEND_PACKET_ERROR);
				break;
			}
			while (ret >= 0)
			{
				ret = avcodec_receive_frame(pThis->m_pvCodecCtx, pFrame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				{
					ret = 0;
					break;
				}
				else if (ret < 0)
					continue;

				//ͼ��ת��
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0,
					pThis->m_pvCodecCtx->height,pFrameYUV420->data,pFrameYUV420->linesize);

				int pts = packet->pts > 0 ? packet->pts : packet->dts;

				//��ǰ֡��ʾʱ��
				double sec = pts * av_q2d(pThis->m_pFormatCtx->streams[pThis->m_videoStream]->time_base);

				{
					std::lock_guard<mutex> mtx_locker(pThis->m_pSizeMutex);
					//ͨ��SDL����ʾ
					SDL_UpdateTexture(pThis->m_sdlTexture, NULL, pFrameYUV420->data[0], pFrameYUV420->linesize[0]);
					SDL_RenderClear(pThis->m_sdlRenderer);
					SDL_RenderCopy(pThis->m_sdlRenderer, pThis->m_sdlTexture, NULL, NULL);
					SDL_RenderPresent(pThis->m_sdlRenderer);
				}

				//���½�����
				pThis->PostMessageW(WM_UPDATE_SLIDER, NULL, sec * 1000);
				int zl = pThis->GetDlgItemInt(IDC_EDIT_ZL);
				//ÿһ֡��ʾʱ��
				Sleep(1000 / zl);
			}
			av_free_packet(packet);
			std::lock_guard<mutex> mtx_locker(pThis->m_VMutex);
			pThis->m_VList.pop();
		}
		else if (pThis->m_bReadPacketEnd)
		{
			//��������Ҷ���Ϊ���򲥷����
			break;
		}
	}

	av_free(buffer);
	av_frame_free(&pFrameYUV420);
	av_frame_free(&pFrame);

	if (pThis->GetPlayType() != EM_PLAY_TYPE_STOP)
	{
		pThis->m_hVPlayHandel = INVALID_HANDLE_VALUE;
		pThis->SetPlayType(EM_PLAY_TYPE_STOP);
	}
	pThis->PostMessage(WM_SHOW_BK, NULL, 0);
	return 0;
}

DWORD WINAPI AudioThread(LPVOID lpParam)
{
	CffmpegTestDlg* pThis = (CffmpegTestDlg*)lpParam;

	do
	{
		CoInitialize(NULL);
		if (!pThis->m_paCodecCtxOrg || !pThis->m_paCodec)
		{
			break;
		}
		if (avcodec_open2(pThis->m_paCodecCtx, pThis->m_paCodec, nullptr) != 0)
		{
			break;
		}

		//����Ĳ�����ʽ
		enum AVSampleFormat in_sample_fmt = pThis->m_paCodecCtx->sample_fmt;
		//����Ĳ�����ʽ 16bit PCM
		enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
		//����Ĳ�����
		int in_sample_rate = pThis->m_paCodecCtx->sample_rate;
		//����Ĳ�����
		int out_sample_rate = in_sample_rate;
		//�������������
		uint64_t in_ch_layout = av_get_default_channel_layout(pThis->m_paCodecCtx->channels);
		//�������������
		uint64_t out_ch_layout = av_get_default_channel_layout(2);

		//���ͨ����
		int out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
		int out_nb_samples = pThis->m_paCodecCtx->frame_size;

		//SDL_AudioSpec
		SDL_AudioSpec wanted_spec;
		wanted_spec.freq = out_sample_rate;
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.channels = out_channels;
		wanted_spec.silence = 0;
		wanted_spec.samples = out_nb_samples;
		wanted_spec.callback = SDL_AudioCallPlay;
		wanted_spec.userdata = pThis;

		if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
		{
			pThis->PostMessageW(WM_ERROR_MSG,EM_SDL_OPENAUDIO_ERROR);
			break;
		}

		//Swr
		struct SwrContext *au_convert_ctx;
		au_convert_ctx = swr_alloc();
		au_convert_ctx = swr_alloc_set_opts(
			au_convert_ctx, 
			out_ch_layout, 
			out_sample_fmt,
			out_sample_rate,
			in_ch_layout, 
			pThis->m_paCodecCtx->sample_fmt, 
			pThis->m_paCodecCtx->sample_rate,
			0, 
			NULL);
		swr_init(au_convert_ctx);

		//Play
		SDL_PauseAudio(0);

		int ret = 0;
		AVPacket packet;
		uint8_t *out_buffer = (uint8_t *)av_malloc(2 * out_sample_rate);
		AVFrame* pFrame = av_frame_alloc();

		while (true)
		{
			SetEvent(pThis->m_hAudioEvent);
			WaitForSingleObject(pThis->m_hSupendEvent, INFINITE);
			ResetEvent(pThis->m_hAudioEvent);
			if (pThis->GetPlayType() == EM_PLAY_TYPE_STOP)
			{
				av_seek_frame(pThis->m_pFormatCtx, pThis->m_AudioStream, 0, AVSEEK_FLAG_BACKWARD);
				break;
			}


			AVPacket* packet = nullptr;
			{
				std::lock_guard<mutex> mtx_locker(pThis->m_AMutex);
				if (!pThis->m_AList.empty())
				{
					packet = &pThis->m_AList.front();
					if (packet->buf == NULL)
					{
						av_free_packet(packet);
						pThis->m_AList.pop_front();
						continue;
					}
				}
				else
					continue;
			}

			ret = avcodec_send_packet(pThis->m_paCodecCtx, packet);
			if (ret < 0)
			{
				pThis->PostMessage(WM_ERROR_MSG, NULL, EM_SEND_PACKET_ERROR);
				break;
			}
			while (ret >= 0)
			{
				ret = avcodec_receive_frame(pThis->m_paCodecCtx, pFrame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				{
					break;
				}
				else if (ret < 0)
					continue;
				else
				{
					int nRet = swr_convert(au_convert_ctx, &out_buffer, 2 * out_sample_rate, (const uint8_t**)pFrame->data, pFrame->nb_samples);
					int buf_len = nRet * out_channels * GetSampleCount(out_sample_fmt);

					ST_ABUF st_buf;
					st_buf.buf = new Uint8[2 * out_sample_rate];
					memset(st_buf.buf, 0, 2 * out_sample_rate);
					memcpy(st_buf.buf, out_buffer, buf_len);
					st_buf.buf_len = buf_len;
					st_buf.pts = packet->pts > 0 ? packet->pts : packet->dts;

					//���������Ƶ���ݷ������
					std::lock_guard<mutex> mtx_locker(pThis->m_CMutex);
					pThis->m_qAbuf.push(st_buf);
				}
			}

			av_free_packet(packet);
			std::lock_guard<mutex> mtx_locker(pThis->m_AMutex);
			pThis->m_AList.pop_front();
		}
		av_free(out_buffer);
		SDL_CloseAudio();
		swr_free(&au_convert_ctx);
	} while (0);
	return 0;
}

//������
void CffmpegTestDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl* pSlider = (CSliderCtrl*)pScrollBar;
	if (pSlider == &m_slider)
	{
		int npos = m_slider.GetPos();
		double all_sec = (double)npos / 1000;//��ǰ����Ϊ������
		int sec = (int)all_sec % 60;
		int min = ((int)all_sec / 60) % 60;
		int h   = all_sec / 3600;
		int ms = (all_sec - (int)all_sec) * 100;
		CString str;
		str.Format(L"%02d:%02d:%02d:%02d", h,min,sec,ms);
		SetDlgItemTextW(IDC_STA_TIME, str);

		if (GetPlayType() == EM_PLAY_TYPE_STOP)
		{
			return;
		}
		else if (GetPlayType() == EM_PLAY_TYPE_PLAY)
		{
			//�϶�������ǰ����ͣ
			OnBnClickedBtnPause();
		}

		//�ȴ������̺߳���Ƶ�ص�����ͣ�������ִ�У������ͻ
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hDecodeEvent, 20)
			&& WAIT_OBJECT_0 == WaitForSingleObject(m_hVideoEvent, 20)
			&& WAIT_OBJECT_0 == WaitForSingleObject(m_hAudioEvent, 20)
			&& WAIT_OBJECT_0 == WaitForSingleObject(m_hACallEvent, 20))
		{
			//��ն���
			ClearQueue();

			uint8_t* buffer = nullptr;
			AVFrame* pFrame = av_frame_alloc();
			AVFrame* pFrameYUV420 = av_frame_alloc();

			do
			{
				// ʹ�õĻ������Ĵ�С
				int numBytes = 0;

				numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, m_pvCodecCtx->width, m_pvCodecCtx->height);
				buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

				avpicture_fill((AVPicture*)pFrameYUV420, buffer, AV_PIX_FMT_YUV420P, m_pvCodecCtx->width, m_pvCodecCtx->height);

				struct SwsContext* sws_ctx = nullptr;
				sws_ctx = sws_getContext(m_pvCodecCtx->width, m_pvCodecCtx->height, m_pvCodecCtx->pix_fmt,
					m_pvCodecCtx->width, m_pvCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);

				AVPacket packet;
				int64_t timestamp = all_sec * AV_TIME_BASE;
				int64_t seek_pts = all_sec / av_q2d(m_pFormatCtx->streams[m_videoStream]->time_base);

				if (av_seek_frame(m_pFormatCtx, m_videoStream, seek_pts, AVSEEK_FLAG_BACKWARD) < 0)
				{
					MessageBox(L"av_seek_frame error", L"error");
					break;
				}
				while (av_read_frame(m_pFormatCtx, &packet) >= 0)
				{
					bool bGet = false;
					if (packet.stream_index == m_videoStream)
					{
						int ret = avcodec_send_packet(m_pvCodecCtx, &packet);
						if (ret < 0)
						{
							av_free_packet(&packet);
							MessageBox(L"Error sending a packet for decoding", L"error");
							break;
						}

						while (ret >= 0)
						{
							ret = avcodec_receive_frame(m_pvCodecCtx, pFrame);
							if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
								break;
							else if (ret < 0)
							{
								av_free_packet(&packet);
								av_free(buffer);
								av_frame_free(&pFrameYUV420);
								av_frame_free(&pFrame);
								MessageBox(L"Error during decoding", L"error");
								return;
							}

							int64_t tmp_pts = packet.pts > 0 ? packet.pts : packet.dts;
							//����һ��ʱ�䷶Χ�඼��seek����ͬ�Ĺؼ�֡��I֡��,��˼���ѭ��ֱ��packet��pts ���� seek_pts����seek_pts���
							if (tmp_pts >= seek_pts)
							{
								//ͼ��ת��
								sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0,
									m_pvCodecCtx->height, pFrameYUV420->data, pFrameYUV420->linesize);

								//ͨ��SDL����ʾ
								SDL_UpdateTexture(m_sdlTexture, NULL, pFrameYUV420->data[0], pFrameYUV420->linesize[0]);
								SDL_RenderClear(m_sdlRenderer);
								SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, NULL, NULL);
								SDL_RenderPresent(m_sdlRenderer);
								bGet = true;
							}
						}
						if (bGet)
						{
							av_free_packet(&packet);
							break;
						}
					}
					av_free_packet(&packet);
				}
			} while (0);
			av_free(buffer);
			av_frame_free(&pFrameYUV420);
			av_frame_free(&pFrame);
		}
	}
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

//��
void CffmpegTestDlg::OnBnClickedBtnOpen()
{
	USES_CONVERSION;
	CString strTmp;
	CString strFile = _T("");
	CFileDialog    dlgFile(TRUE, NULL, NULL, OFN_HIDEREADONLY, _T("All Files (*.*)|*.*||"), NULL);
	if (dlgFile.DoModal() == IDOK)
	{
		//���ͷ�
		FreeVariable();
		strFile = dlgFile.GetPathName();
		GetDlgItem(IDC_EDIT_FILE)->SetWindowTextW(strFile);

		// ��ȡ�ļ�ͷ������ʽ�����Ϣ�����AVFormatContext�ṹ����
		if (avformat_open_input(&m_pFormatCtx, T2A(strFile), nullptr, nullptr) != 0)
		{
			MessageBox(L"avformat open input failed !", L"����");
			return;
		}

		// ����ļ�������Ϣ
		if (avformat_find_stream_info(m_pFormatCtx, nullptr) < 0)
		{
			MessageBox(L"avformat find stream info failed !", L"����");
			return;
		}

		//������Ƶ�� ��Ƶ��
		for (int i = 0; i < m_pFormatCtx->nb_streams; i++)
		{
			if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
				m_videoStream = i;
			else if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
				m_AudioStream = i;
		}
		if (m_videoStream == -1)
		{
			MessageBox(L"û���ҵ���Ƶ��!", L"����");
			return;
		}

		//video
		AVStream* vStream = m_pFormatCtx->streams[m_videoStream];
		m_pvCodecCtxOrg = vStream->codec;

		// �ҵ���Ƶ������
		m_pvCodec = avcodec_find_decoder(m_pvCodecCtxOrg->codec_id);
		if (!m_pvCodec)
		{
			MessageBox(L"������Ƶ������ʧ��!", L"����");
			return;
		}

		// ��ֱ��ʹ�ô�AVFormatContext�õ���CodecContext��Ҫ����һ��
		m_pvCodecCtx = avcodec_alloc_context3(m_pvCodec);
		if (avcodec_copy_context(m_pvCodecCtx, m_pvCodecCtxOrg) != 0)
		{
			MessageBox(L"Could not copy vcodec context!", L"����");
			return;
		}

		//audio
		if (m_AudioStream >= 0)
		{
			AVStream* aStream = m_pFormatCtx->streams[m_AudioStream];
			m_paCodecCtxOrg = aStream->codec;

			// �ҵ���Ƶ������
			m_paCodec = avcodec_find_decoder(m_paCodecCtxOrg->codec_id);
			if (!m_paCodec)
			{
				MessageBox(L"������Ƶ������ʧ��!", L"����");
				return;
			}

			m_paCodecCtx = avcodec_alloc_context3(m_paCodec);
			if (avcodec_copy_context(m_paCodecCtx, m_paCodecCtxOrg) != 0)
			{
				MessageBox(L"Could not copy acodec context!", L"����");
				return;
			}

			//������
			int channel = m_paCodecCtx->channels;
			strTmp.Format(L"%d", channel);
			GetDlgItem(IDC_EDIT_SDS)->SetWindowText(strTmp);
			//������
			int rate = m_paCodecCtx->sample_rate;
			strTmp.Format(L"%d", rate);
			GetDlgItem(IDC_EDIT_RATE)->SetWindowText(strTmp);
		}
		else
		{
			GetDlgItem(IDC_EDIT_SDS)->SetWindowText(L"");
			GetDlgItem(IDC_EDIT_RATE)->SetWindowText(L"");
		}

		double time = (double)(m_pFormatCtx->duration) / AV_TIME_BASE;//ʱ��
		int us = (m_pFormatCtx->duration) % AV_TIME_BASE;
		int sec = (int)time % 60;
		int min = (int)time / 60 % 60;
		int h = (int)time / 60 / 60;
		strTmp.Format(L"%02dʱ%02d��%02d��%02d", h, min, sec, (100 * us) / AV_TIME_BASE);
		GetDlgItem(IDC_EDIT_TIME)->SetWindowTextW(strTmp);

		int ml = m_pFormatCtx->bit_rate;//����
		strTmp.Format(L"%d", ml);
		GetDlgItem(IDC_EDIT_ML)->SetWindowTextW(strTmp);

		AVInputFormat* avPtFmt = m_pFormatCtx->iformat;
		GetDlgItem(IDC_EDIT_FORMAT_NAME)->SetWindowTextW(A2T(avPtFmt->name));
		GetDlgItem(IDC_EDIT_FORMAT_LNAME)->SetWindowTextW(A2T(avPtFmt->long_name));
		GetDlgItem(IDC_EDIT_FORMAT_EXNAME)->SetWindowTextW(A2T(avPtFmt->extensions));

		int frame_rate = 0;//֡��
		if (vStream->r_frame_rate.den > 0)
			frame_rate = av_q2d(vStream->r_frame_rate);//avStream->r_frame_rate.num / avStream->r_frame_rate.den;
		else if (m_pvCodecCtxOrg->framerate.den > 0)
			frame_rate = av_q2d(m_pvCodecCtxOrg->framerate);//m_pvCodecCtxOrg->framerate.num / m_pvCodecCtxOrg->framerate.den;

		int zs = vStream->nb_frames;
		if (zs == 0)
		{
			zs = time * frame_rate;
		}
		strTmp.Format(L"%d", zs);
		GetDlgItem(IDC_EDIT_ZS)->SetWindowTextW(strTmp);
		strTmp.Format(L"%d", frame_rate);
		GetDlgItem(IDC_EDIT_ZL)->SetWindowTextW(strTmp);
		strTmp.Format(L"%d", m_pvCodecCtxOrg->width);
		GetDlgItem(IDC_EDIT_WIDTH)->SetWindowTextW(strTmp);
		strTmp.Format(L"%d", m_pvCodecCtxOrg->height);
		GetDlgItem(IDC_EDIT_HEIGHT)->SetWindowTextW(strTmp);

		m_slider.SetRange(0, time * 1000);
		m_slider.SetTicFreq(1);
		m_slider.SetPos(0);
	}
}

//����
void CffmpegTestDlg::OnBnClickedPlay()
{
	if (m_playType == EM_PLAY_TYPE_PAUSE)
	{
		SetPlayType(EM_PLAY_TYPE_PLAY);
	}
	else if (m_playType == EM_PLAY_TYPE_STOP)
	{
		SetPlayType(EM_PLAY_TYPE_PLAY);
		m_hDecodeHandel = CreateThread(NULL, NULL, DecodeThread, this, 0, 0);
	}
}

//��ͣ
//m_hDecodeEvent,m_hVideoEvent,m_hAudioEvent�����ź�ʱ�ű�ʾ������ͣ��
void CffmpegTestDlg::OnBnClickedBtnPause()
{
	SetPlayType(EM_PLAY_TYPE_PAUSE);
}

//ֹͣ
void CffmpegTestDlg::OnBnClickedBtnStop()
{
	SetPlayType(EM_PLAY_TYPE_STOP);
}


void CffmpegTestDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	SetPlayType(EM_PLAY_TYPE_STOP);
	if (m_hDecodeHandel != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(m_hDecodeHandel, INFINITE);
		m_hDecodeHandel = INVALID_HANDLE_VALUE;
	}
	if (m_hVPlayHandel != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(m_hVPlayHandel, INFINITE);
		m_hVPlayHandel = INVALID_HANDLE_VALUE;
	}
	if (m_hAPlayHandel != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(m_hAPlayHandel, INFINITE);
		m_hAPlayHandel = INVALID_HANDLE_VALUE;
	}
	FreeVariable();
	if (m_sdlWindow)
	{
		SDL_DestroyWindow(m_sdlWindow);
	}
	SDL_Quit();
}


void CffmpegTestDlg::OnNMReleasedcaptureSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (GetPlayType() == EM_PLAY_TYPE_PAUSE)
	{
		OnBnClickedPlay();
	}
	*pResult = 0;
}


void CffmpegTestDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	CRect rectThis;
	GetClientRect(&rectThis);
	if (m_group && m_pic)
	{
		CRect pRect;
		m_pic.GetClientRect(pRect);
		CRect gRect;
		m_group.GetClientRect(gRect);
		pRect.left = gRect.right + 5;
		pRect.top = 40;
		pRect.bottom = rectThis.bottom - 80;
		pRect.right = rectThis.right - 5;
		if (pRect.Width() >= 200 && pRect.Height() >= 200)//ͼƬ�ؼ���С��Χ
		{
			std::lock_guard<mutex> mtx_locker(m_pSizeMutex);
			m_pic.MoveWindow(&pRect, TRUE);
		}
	}
}
