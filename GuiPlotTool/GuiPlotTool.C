#include <TGClient.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>

using namespace std;

class MyMainFrame {
    RQ_OBJECT("MyMainFrame")

public:
    MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~MyMainFrame();

    void LoadAllPlotsFromDir(TDirectory *src);
    void DisplayMapInListBox(const map<string, TObject *> &m, TGListBox *listbox);
    void FilterBySearchBox();

    void RemoveFromSelection();
    void AddToSelection();

    void PreviewSelection();
    void PreviewSelectedItem();
    void Superimpose();

    void Clear();
    void testfunction();

private:
    TGMainFrame* fMain;
    TGTextEntry* searchBox;
    TGListBox*   mainListBox;
    TGListBox*   selectionListBox;
//    TGLabel*     topDirectoryIndicator;

    TFile*      file;
    TDirectory* baseDir;

    TCanvas* previewCanvas;
    TCanvas* resultCanvas;

    map<string, TObject *> table;
    map<string, TObject *> selection;

};


MyMainFrame::MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h) {
    fMain = new TGMainFrame(p, w, h);

//    topDirectoryIndicator = new TGLabel(fMain);
    mainListBox = new TGListBox(fMain, -1, kSunkenFrame);
    selectionListBox = new TGListBox(fMain, -1, kSunkenFrame);
    searchBox = new TGTextEntry(fMain);

    searchBox->Connect("TextChanged(const char *)", "MyMainFrame", this, "FilterBySearchBox()");
    mainListBox->Connect("DoubleClicked(Int_t, Int_t)", "MyMainFrame", this, "AddToSelection()");
    selectionListBox->Connect("DoubleClicked(Int_t, Int_t)", "MyMainFrame", this, "RemoveFromSelection()");

//    TGHorizontalFrame* searchFrame = new TGHorizontalFrame(fMain, 200, 20);
    TGHorizontalFrame* mainFrame = new TGHorizontalFrame(fMain, 200, 20);
    TGHorizontalFrame* selectFrame = new TGHorizontalFrame(fMain, 200, 40);


    // --- top frame
    TGTextButton* previewSelected = new TGTextButton(mainFrame, "&Preview Item");
    previewSelected->Connect("Clicked()", "MyMainFrame", this, "PreviewSelectedItem()");
    mainFrame->AddFrame(previewSelected, new TGLayoutHints(kLHintsCenterX, 4, 4, 3, 4));

    // --- lower buttons
    TGTextButton* draw = new TGTextButton(selectFrame, "&Preview Selection List");
    draw->Connect("Clicked()", "MyMainFrame", this, "PreviewSelection()");
    selectFrame->AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    TGTextButton* superimpose = new TGTextButton(selectFrame, "&Superimpose Selection");
    superimpose->Connect("Clicked()", "MyMainFrame", this, "Superimpose()");
    selectFrame->AddFrame(superimpose, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    TGTextButton* clear = new TGTextButton(selectFrame, "&Clear All");
    clear->Connect("Clicked()", "MyMainFrame", this, "Clear()");
    selectFrame->AddFrame(clear, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    fMain->AddFrame(searchBox, new TGLayoutHints(kLHintsExpandX, 5, 5, 5, 6));
    fMain->AddFrame(mainFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    fMain->AddFrame(mainListBox, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 6));
    fMain->AddFrame(selectionListBox, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 6, 7));
    fMain->AddFrame(selectFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    fMain->SetWindowName("Filter & Combine Plots");
    fMain->MapSubwindows();
    mainListBox->Resize(200, 300);
    mainListBox->MoveResize(250, 80, 150, 250);

    selectionListBox->Resize(200, 300);
    selectionListBox->MoveResize(250, 280, 150, 300);

    fMain->MoveResize(200, 300, 500, 600);

    // Map main frame
    fMain->MapWindow();

    file = TFile::Open("/home/fil/projects/PR/root/GuiPlotTool/DQM_V0001_SiStrip_R000283283.root");

    string baseDir_str = "DQMData/Run 283283/SiStrip/Run summary/MechanicalView";
    baseDir = (TDirectory *)file->Get(baseDir_str.c_str());

    if (baseDir == nullptr) {
        cout << "Error while opening File " << endl;
    }

    LoadAllPlotsFromDir(baseDir);
    DisplayMapInListBox(table, mainListBox);

    fMain->MoveResize(200, 300, 1200, 600);
}

MyMainFrame::~MyMainFrame() {
    fMain->Cleanup();
    delete fMain;
}

void MyMainFrame::testfunction() {

cout << "Holy shit " << endl;
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
            table.insert(pair<string, TObject *>(path, h));
        }
    }
}

void MyMainFrame::DisplayMapInListBox(const map<string, TObject *> &m, TGListBox *listbox) {
    listbox->RemoveAll();
    for (auto &elem : m) {
        listbox->AddEntry(elem.first.c_str(), listbox->GetNumberOfEntries());
    }
    listbox->SortByName();
}

void MyMainFrame::AddToSelection() {
    string key = mainListBox->GetSelectedEntry()->GetTitle();
    TObject *val = table.find(key)->second;
    selection.insert(pair<string, TObject *>(key, val));
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::RemoveFromSelection() {
    string key = selectionListBox->GetSelectedEntry()->GetTitle();
    selection.erase(key);
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::Clear() {
    selection.clear();
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::FilterBySearchBox() {
    mainListBox->RemoveAll();
    map<string, TObject *> tmp_filtered;
    string seach_text = searchBox->GetText();

    for (auto &elem : table) {
        if (elem.first.find(seach_text) != string::npos) {
            string key = elem.first;
            TObject *val = elem.second;
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
        elem.second->Draw();
    }
}

void MyMainFrame::PreviewSelectedItem() {
    previewCanvas = new TCanvas("Preview Canvas", "", 800, 400);
    string key = mainListBox->GetSelectedEntry()->GetTitle();
    TObject *val = table.find(key)->second;

    previewCanvas->cd();
    val->Draw();
}

void MyMainFrame::Superimpose() {
    resultCanvas = new TCanvas("Superimpose Canvas", "", 800, 400);
    resultCanvas->cd();

    for(auto& elem : selection){
        ((TH1*)elem.second)->SetStats(false);
        elem.second->Draw("same");
    }
}

void GuiPlotTool() {
    new MyMainFrame(gClient->GetRoot(), 200, 200);
}
