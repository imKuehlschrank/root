#include <TGClient.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>


//OnTrack__TID__PLUS__ring__

using namespace std;

class HistogramInfo {
public:
    HistogramInfo(TObject* o, string p, string n) : obj(o), filePath(p), name(n) {}

    TObject* GetObj()  const { return obj; }
    string   GetPath() const { return filePath; }
    string   GetName() const { return name; }

private:
    TObject* obj;
    string filePath;
    string name;
};

class MyMainFrame {
    RQ_OBJECT("MyMainFrame")

public:
    MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~MyMainFrame();

    void LoadAllPlotsFromDir(TDirectory *src);
    void DisplayMapInListBox(const map<Int_t, HistogramInfo> &m, TGListBox *listbox);
    void FilterBySearchBox();

    void RemoveFromSelection();
    void AddToSelection();

    void PreviewSelection();
    void Superimpose();

    void Clear();
    void Toggle_path_display();

private:
    TGMainFrame* fMain;
    TGTextEntry* searchBox;
    TGListBox*   mainListBox;
    TGListBox*   selectionListBox;

    TFile*      file;
    TDirectory* baseDir;

    TGLabel* currdirLabel;

    TCanvas* previewCanvas;
    TCanvas* resultCanvas;

    map<Int_t, HistogramInfo> table;
    map<Int_t, HistogramInfo> selection;

    TGFileDialog* loadDialog;

    Int_t freeId = 0;
    Int_t GetNextFreeId() { return freeId++; }

    bool display_full_path = false;
    void InitAll();
};


MyMainFrame::MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h) {
    fMain = new TGMainFrame(p, w, h);

    mainListBox = new TGListBox(fMain, -1, kSunkenFrame);
    selectionListBox = new TGListBox(fMain, -1, kSunkenFrame);
    //    searchBox = new TGTextEntry(fMain);

    TGHorizontalFrame* mainFrame = new TGHorizontalFrame(fMain, 200, 20);
    TGHorizontalFrame* selectFrame = new TGHorizontalFrame(fMain, 200, 40);

    currdirLabel = new TGLabel(fMain,"");
    fMain->AddFrame(currdirLabel, new TGLayoutHints(kLHintsCenterX ,2,2,2,2));
    currdirLabel->MoveResize(0,0,500,16);


    // "Quicksearch" group frame
    TGGroupFrame *quicksearchFrame = new TGGroupFrame(fMain,"Quicksearch");
    quicksearchFrame->SetLayoutBroken(kTRUE);

    searchBox = new TGTextEntry(quicksearchFrame);
    searchBox->SetAlignment(kTextLeft);
    searchBox->SetText("");

    quicksearchFrame->AddFrame(searchBox, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX ,2,2,2,2));
    searchBox->MoveResize(120,16,300,20);

    TGLabel *substrLabel = new TGLabel(quicksearchFrame,"Enter Substring:");
    quicksearchFrame->AddFrame(substrLabel, new TGLayoutHints(kLHintsLeft ,2,2,2,2));
    substrLabel->MoveResize(8,18,100,16);

    TGCheckButton *displayPathCheckBox = new TGCheckButton(quicksearchFrame,"Display path");
    quicksearchFrame->AddFrame(displayPathCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    displayPathCheckBox->MoveResize(8,40,100,17);

    quicksearchFrame->SetLayoutManager(new TGVerticalLayout(quicksearchFrame));
    quicksearchFrame->Resize(608,72);

    // botom part
    TGTextButton* draw = new TGTextButton(selectFrame, "&Preview Selection List");
    draw->Connect("Clicked()", "MyMainFrame", this, "PreviewSelection()");
    selectFrame->AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    TGTextButton* superimpose = new TGTextButton(selectFrame, "&Superimpose Selection");
    superimpose->Connect("Clicked()", "MyMainFrame", this, "Superimpose()");
    selectFrame->AddFrame(superimpose, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    TGTextButton* clear = new TGTextButton(selectFrame, "&Clear Selection");
    clear->Connect("Clicked()", "MyMainFrame", this, "Clear()");
    selectFrame->AddFrame(clear, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    fMain->AddFrame(mainFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    fMain->AddFrame(quicksearchFrame, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
    fMain->AddFrame(mainListBox, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 6));
    fMain->AddFrame(selectionListBox, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 6, 7));
    fMain->AddFrame(selectFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    fMain->SetWindowName("Filter & Combine Plots");
    fMain->MapSubwindows();

    searchBox->Connect("TextChanged(const char *)", "MyMainFrame", this, "FilterBySearchBox()");
    mainListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "AddToSelection()");
    selectionListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "RemoveFromSelection()");
    displayPathCheckBox->Connect("Clicked()", "MyMainFrame", this, "Toggle_path_display()");

    // Map main frame
    fMain->MapWindow();
    fMain->MoveResize(200, 300, 450, 600);
    InitAll();
}

MyMainFrame::~MyMainFrame() {
    fMain->Cleanup();
    delete fMain;
}

void MyMainFrame::InitAll(){
    TGFileInfo file_info_;
    const char *filetypes[] = {"ROOT files", "*.root", 0, 0};
    file_info_.fFileTypes = filetypes;
    new TGFileDialog(gClient->GetDefaultRoot(), fMain, kFDOpen, &file_info_);

    file = TFile::Open(file_info_.fFilename);

    // string baseDirStr = "DQMData/Run 283283/SiStrip/Run summary/MechanicalView";
    string baseDirStr = "DQMData";
    baseDir = (TDirectory *)file->Get(baseDirStr.c_str());

    if (baseDir == nullptr) {
        cout << "Error while opening File " << endl;
    }

    string currentPath = string(file_info_.fFilename) + ":/" + string(baseDirStr);
    currdirLabel->SetText(currentPath.c_str());

    LoadAllPlotsFromDir(baseDir);
    DisplayMapInListBox(table, mainListBox);
    fMain->MoveResize(200, 300, 450, 600);
}

void MyMainFrame::Toggle_path_display() {
    display_full_path = !display_full_path;
    DisplayMapInListBox(table, mainListBox);
    DisplayMapInListBox(selection, selectionListBox);
    FilterBySearchBox();
}

void MyMainFrame::LoadAllPlotsFromDir(TDirectory *current) {
    TIter next(current->GetListOfKeys());
    TKey *key;

    while ((key = (TKey *)next())) {
        TClass *cl = gROOT->GetClass(key->GetClassName());

        if (cl->InheritsFrom("TDirectory")) {
            TDirectory *d = (TDirectory *)key->ReadObj();
            LoadAllPlotsFromDir(d);
        }

        if (cl->InheritsFrom("TH1")) {
            TH1 *h = (TH1 *)key->ReadObj();
            string fname = h->GetName();
            string path = string(current->GetPath()) + "/" + fname;

            Int_t key_entry = GetNextFreeId();
            HistogramInfo value_entry(h, path, fname);

            table.insert(make_pair(key_entry, value_entry));
        }
    }
}

void MyMainFrame::DisplayMapInListBox(const map<Int_t, HistogramInfo> &m, TGListBox *listbox) {
    listbox->RemoveAll();

    for (auto &elem : m) {
        if(display_full_path)
            listbox->AddEntry(elem.second.GetPath().c_str(), elem.first);
        else
            listbox->AddEntry(elem.second.GetName().c_str(), elem.first);
    }

    listbox->SortByName();
}

void MyMainFrame::AddToSelection() {
    Int_t key = mainListBox->GetSelected();
    HistogramInfo val = table.find(key)->second;

    selection.insert(make_pair(key, val));
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::RemoveFromSelection() {
    Int_t key = selectionListBox->GetSelected();
    selection.erase(key);
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::Clear() {
    selection.clear();
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::FilterBySearchBox() {
    mainListBox->RemoveAll();

    map<Int_t, HistogramInfo> tmp_filtered;

    string seach_text = searchBox->GetText();

    for (auto &elem : table) {
        string fullname = elem.second.GetPath();
        if (fullname.find(seach_text) != string::npos) {
            tmp_filtered.insert(elem);
        }
    }
    DisplayMapInListBox(tmp_filtered, mainListBox);
}

void MyMainFrame::PreviewSelection() {
    previewCanvas = new TCanvas("Preview Canvas", "", 800, 400);
    previewCanvas->Divide(selection.size(), 1);

    Int_t i = 1;
    for (auto &elem : selection) {
        previewCanvas->cd(i++);
        elem.second.GetObj()->Draw();
    }
}

void MyMainFrame::Superimpose() {
    resultCanvas = new TCanvas("Superimpose Canvas", "", 800, 400);
    resultCanvas->cd();

    for(auto& elem : selection){
        ((TH1*)elem.second.GetObj())->SetStats(false);
        elem.second.GetObj()->Draw("same");
    }
}

void GuiPlotTool() {
    new MyMainFrame(gClient->GetRoot(), 200, 200);
}
