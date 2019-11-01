
// ffmpegTestDlg.h : 头文件
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

#define  WM_SHOW_BK WM_USER+10 //显示默认背景消息
#define  WM_UPDATE_SLIDER WM_USER+11 //更新进度条消息
#define  WM_ERROR_MSG WM_USER+12 //错误消息
#define  MAX_VQUEUE 300 //图片帧队列饱和值

//错误类型
enum EM_ERROR_TYPE
{
	EM_SEND_PACKET_ERROR,
	EM_AVCODE_OPEN_ERROR,
	EM_SDL_OPENAUDIO_ERROR,
	EM_ERROR_MAX

};

//播放状态
enum EM_PLAY_TYPE
{
	EM_PLAY_TYPE_PLAY,//播放
	EM_PLAY_TYPE_PAUSE,//暂停
	EM_PLAY_TYPE_STOP,//停止
	EM_PLAY_TYPE_MAX
};

struct ST_ABUF
{
	Uint8* buf;
	int buf_len;
	int pts;//当前播放声音的时间戳
};

// CffmpegTestDlg 对话框
class CffmpegTestDlg : public CDialogEx
{
// 构造
public:
	CffmpegTestDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CffmpegTestDlg();
// 对话框数据
	enum { IDD = IDD_FFMPEGTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedOk();
	afx_msg HRESULT OnShowBk(WPARAM wParam, LPARAM lParam);//显示默认背景图消息
	afx_msg HRESULT OnUpdateSlider(WPARAM wParam, LPARAM lParam);//更新进度条消息
	afx_msg HRESULT OnErrorMsg(WPARAM wParam, LPARAM lParam);//错误消息
	afx_msg void OnBnClickedBtnOpen();//打开文件,获取基本的文件信息
	afx_msg void OnBnClickedPlay();//开始播放
	afx_msg void OnBnClickedBtnPause();//暂停
	afx_msg void OnBnClickedBtnStop();//停止
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);//拖动进度条消息
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnNMReleasedcaptureSlider(NMHDR *pNMHDR, LRESULT *pResult);//拖动进度条后释放消息
	DECLARE_MESSAGE_MAP()

private:
	std::atomic<EM_PLAY_TYPE>     m_playType;//播放状态

public:
	//释放变量
	void FreeVariable();
	//显示默认背景图片
	void showBk(CString strPath = L"bk.bmp");
	//保存图片
	void saveFrame(AVFrame* pFrame, int width, int height, int iFrame);
	//设置播放状态
	void SetPlayType(EM_PLAY_TYPE type);
	//获取播放状态
	EM_PLAY_TYPE GetPlayType();
	//清空队列
	void ClearQueue();

public:
	HANDLE           m_hSupendEvent;//暂停信号
	HANDLE           m_hDecodeEvent;//DecodeThread暂停标志
	HANDLE           m_hVideoEvent;//VideoThread暂停标志
	HANDLE           m_hAudioEvent;//AudioThread暂停标志
	HANDLE           m_hACallEvent;//AudioCallPlay暂停标志
	HANDLE           m_hDecodeHandel;//解码线程句柄
	HANDLE           m_hVPlayHandel;//视频播放线程句柄
	HANDLE           m_hAPlayHandel;//音频播放线程句柄

	//av_read_frame已经读到最后则m_bReadPacketEnd = true，若此时播放还未结束，将进度条向前拉，则m_bReadPacketEnd = false
	bool             m_bReadPacketEnd;

	AVFormatContext* m_pFormatCtx;

	//video
	int              m_videoStream;//视频流索引
	AVCodecContext*  m_pvCodecCtxOrg;
	AVCodecContext*  m_pvCodecCtx;
	AVCodec*         m_pvCodec;
	queue<AVPacket>  m_VList;//存放视频的AVPacket
	mutex            m_VMutex;//对m_VList操作的互斥锁
	mutex            m_pSizeMutex;//对改变窗口大小的互斥锁

	//audio
	int              m_AudioStream;//音频流索引
	AVCodecContext*  m_paCodecCtxOrg;
	AVCodecContext*  m_paCodecCtx;
	AVCodec*         m_paCodec;
	queue<ST_ABUF>   m_qAbuf;//声音队列，存放解码后的数据
	list<AVPacket>   m_AList;//存放音频的AVPacket
	mutex            m_AMutex;//对m_AList操作的互斥锁
	mutex            m_CMutex;//对m_qAbuf操作的互斥锁

	//sdl
	SDL_Window*      m_sdlWindow;
	SDL_Renderer*    m_sdlRenderer;
	SDL_Texture*     m_sdlTexture;

	//控件
	CStatic          m_pic;
	CEdit            m_editFile;
	CSliderCtrl      m_slider;
	CStatic          m_group;
};
