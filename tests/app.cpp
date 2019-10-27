#include <wx/wx.h>

// アプリケーションについての挙動を管理するシングルトンクラス。
// 具体的には、初期化処理、メインループの管理など。
class MyApp : public wxApp
{
    virtual bool OnInit() override;
};

// wxGetApp 関数を宣言する（後述）
wxDECLARE_APP(MyApp);

// メインウィンドウを表すクラス。
class MyFrame : public wxFrame
{
public:
    MyFrame();
    virtual ~MyFrame();

    void OnClose(wxCloseEvent& event);
};

// アプリケーションの初期化処理。
// 実質的な main 関数の代わりと思って良い。
// true を返した場合はメインループに突入し、 false を返した場合はそのまま終了する。
bool MyApp::OnInit()
{
    // コマンドライン引数を処理する。
    if(!wxApp::OnInit())
        return false;

    MyFrame* frame = new MyFrame;
    frame->Show();

    return true;
}

MyFrame::MyFrame()
    : wxFrame(nullptr, wxID_ANY, "Minimal App")
{
    // メインメニューの作成
    auto menuBar = new wxMenuBar;
    auto menuFile = new wxMenu;
    menuFile->Append(wxID_EXIT, "Quit"); // wxID_EXIT のような定義済みIDはプラットフォームによって扱いが変わる。例えば、 Mac ではアプリケーションメニューの Quit と統合される。
    menuBar->Append(menuFile, "File");
    SetMenuBar(menuBar);

    // イベントハンドラーの登録
    // 自身のメンバー関数を登録する場合：
    Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);
    // 無名関数で直接記述する場合：
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);
}

MyFrame::~MyFrame()
{
}

// wxEVT_CLOSE_WINDOW はウィンドウが閉じられようとしていた場合に呼ばれる。
// 実装しなかった場合は単に Destroy() が呼ばれる。
void MyFrame::OnClose(wxCloseEvent& event)
{
    Destroy();
}

// アプリケーションの main 関数がここで定義される（後述）
wxIMPLEMENT_APP(MyApp);