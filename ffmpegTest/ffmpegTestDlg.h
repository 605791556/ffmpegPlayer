
// ffmpegTestDlg.h : ͷ�ļ�
//

#pragma once
#include <atomic>
#include "afxwin.h"
#include "afxcmn.h"
#include <queue>
#include <list>
#include <mutex>

using std::queue;
using std::list;
using std::mutex;

#define LOCK_MUTEX(X)	WaitForSingleObject(X, INFINITE); 
#define UNLOCK_MUTEX(X)	ReleaseMutex(X); 

#define  WM_SHOW_BK WM_USER+10 //��ʾĬ�ϱ�����Ϣ
#define  WM_UPDATE_SLIDER WM_USER+11 //���½�������Ϣ
#define  WM_ERROR_MSG WM_USER+12 //������Ϣ
#define  MAX_VQUEUE 300 //ͼƬ֡���б���ֵ

//��������
enum EM_ERROR_TYPE
{
	EM_SEND_PACKET_ERROR,
	EM_AVCODE_OPEN_ERROR,
	EM_SDL_OPENAUDIO_ERROR,
	EM_ERROR_MAX

};

//����״̬
enum EM_PLAY_TYPE
{
	EM_PLAY_TYPE_PLAY,//����
	EM_PLAY_TYPE_PAUSE,//��ͣ
	EM_PLAY_TYPE_STOP,//ֹͣ
	EM_PLAY_TYPE_MAX
};

struct ST_ABUF
{
	Uint8* buf;
	int buf_len;
	int pts;//��ǰ����������ʱ���
};

// CffmpegTestDlg �Ի���
class CffmpegTestDlg : public CDialogEx
{
// ����
public:
	CffmpegTestDlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CffmpegTestDlg();
// �Ի�������
	enum { IDD = IDD_FFMPEGTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedOk();
	afx_msg HRESULT OnShowBk(WPARAM wParam, LPARAM lParam);//��ʾĬ�ϱ���ͼ��Ϣ
	afx_msg HRESULT OnUpdateSlider(WPARAM wParam, LPARAM lParam);//���½�������Ϣ
	afx_msg HRESULT OnErrorMsg(WPARAM wParam, LPARAM lParam);//������Ϣ
	afx_msg void OnBnClickedBtnOpen();//���ļ�,��ȡ�������ļ���Ϣ
	afx_msg void OnBnClickedPlay();//��ʼ����
	afx_msg void OnBnClickedBtnPause();//��ͣ
	afx_msg void OnBnClickedBtnStop();//ֹͣ
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);//�϶���������Ϣ
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnNMReleasedcaptureSlider(NMHDR *pNMHDR, LRESULT *pResult);//�϶����������ͷ���Ϣ
	DECLARE_MESSAGE_MAP()

private:
	std::atomic<EM_PLAY_TYPE>     m_playType;//����״̬

public:
	//�ͷű���
	void FreeVariable();
	//��ʾĬ�ϱ���ͼƬ
	void showBk(CString strPath = L"bk.bmp");
	//����ͼƬ
	void saveFrame(AVFrame* pFrame, int width, int height, int iFrame);
	//���ò���״̬
	void SetPlayType(EM_PLAY_TYPE type);
	//��ȡ����״̬
	EM_PLAY_TYPE GetPlayType();
	//��ն���
	void ClearQueue();

public:
	HANDLE           m_hSupendEvent;//��ͣ�ź�
	HANDLE           m_hDecodeEvent;//DecodeThread��ͣ��־
	HANDLE           m_hVideoEvent;//VideoThread��ͣ��־
	HANDLE           m_hAudioEvent;//AudioThread��ͣ��־
	HANDLE           m_hACallEvent;//AudioCallPlay��ͣ��־
	HANDLE           m_hDecodeHandel;//�����߳̾��
	HANDLE           m_hVPlayHandel;//��Ƶ�����߳̾��
	HANDLE           m_hAPlayHandel;//��Ƶ�����߳̾��

	//av_read_frame�Ѿ����������m_bReadPacketEnd = true������ʱ���Ż�δ����������������ǰ������m_bReadPacketEnd = false
	bool             m_bReadPacketEnd;

	AVFormatContext* m_pFormatCtx;

	//video
	int              m_videoStream;//��Ƶ������
	AVCodecContext*  m_pvCodecCtxOrg;
	AVCodecContext*  m_pvCodecCtx;
	AVCodec*         m_pvCodec;
	queue<AVPacket>  m_VList;//�����Ƶ��AVPacket
	mutex            m_VMutex;//��m_VList�����Ļ�����
	mutex            m_pSizeMutex;//�Ըı䴰�ڴ�С�Ļ�����

	//audio
	int              m_AudioStream;//��Ƶ������
	AVCodecContext*  m_paCodecCtxOrg;
	AVCodecContext*  m_paCodecCtx;
	AVCodec*         m_paCodec;
	queue<ST_ABUF>   m_qAbuf;//�������У���Ž���������
	list<AVPacket>   m_AList;//�����Ƶ��AVPacket
	mutex            m_AMutex;//��m_AList�����Ļ�����
	mutex            m_CMutex;//��m_qAbuf�����Ļ�����

	//sdl
	SDL_Window*      m_sdlWindow;
	SDL_Renderer*    m_sdlRenderer;
	SDL_Texture*     m_sdlTexture;

	//�ؼ�
	CStatic          m_pic;
	CEdit            m_editFile;
	CSliderCtrl      m_slider;
	CStatic          m_group;
};