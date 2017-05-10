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
    HistogramInfo(TObject* o, string p) : obj(o), filePath(p) {}

    TObject* GetObj()  const { return obj; }
    string   GetPath() const { return filePath; }
    string   GetName() const { return obj->GetName(); }

private:
    TObject* obj;
    string filePath;
};

class MyMainFrame {
    RQ_OBJECT("MyMainFrame")

public:
    MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~MyMainFrame();

    void LoadAllPlotsFromDir(TDirectory *src);
    void DisplayMapInListBox(const map<Int_t, HistogramInfo> &m, TGListBox *listbox);
    void FilterBySearchBox();

    void RemoveFromSelection(Int_t id);
    void AddToSelection(Int_t id);

    void PreviewSelection();
    void Superimpose();

    void ClearSelectionListbox();
    void Toggle_path_display();

    string LoadFileFromDialog();

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

    // #### DESIGN ####
    // ---- Top label
    currdirLabel = new TGLabel(fMain,"");
    currdirLabel->MoveResize(0,0,500,16);

    // ---- Quicksearch Frame
    TGGroupFrame* quicksearchFrame = new TGGroupFrame(fMain,"Quicksearch");

    searchBox = new TGTextEntry(quicksearchFrame);
    TGCheckButton *displayPathCheckBox = new TGCheckButton(quicksearchFrame,"Display path");

    searchBox->MoveResize(120,16,300,20);
    displayPathCheckBox->MoveResize(8,40,100,17);

    quicksearchFrame->AddFrame(searchBox, new TGLayoutHints(kLHintsExpandX ,2,2,2,2));
    quicksearchFrame->AddFrame(displayPathCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

    // ---- Listboxes
    mainListBox = new TGListBox(fMain, -1, kSunkenFrame);
    selectionListBox = new TGListBox(fMain, -1, kSunkenFrame);

    // ---- Control Frame
    TGHorizontalFrame* controlFrame = new TGHorizontalFrame(fMain, 200, 40);

    TGTextButton* previewSelectionButton = new TGTextButton(controlFrame, "&Preview Selection List");
    TGTextButton* superimposeButton = new TGTextButton(controlFrame, "&Superimpose Selection");
    TGTextButton* clearSelectionButton = new TGTextButton(controlFrame, "&Clear Selection");

    controlFrame->AddFrame(previewSelectionButton, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    controlFrame->AddFrame(superimposeButton, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    controlFrame->AddFrame(clearSelectionButton, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));

    // ---- Main Frame, Adding all the individual frames in the appropriate order top to bottom
    fMain->AddFrame(currdirLabel, new TGLayoutHints(kLHintsCenterX ,2,2,2,2));
    fMain->AddFrame(quicksearchFrame, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
    fMain->AddFrame(mainListBox, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 6));
    fMain->AddFrame(selectionListBox, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 6, 7));
    fMain->AddFrame(controlFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    fMain->SetWindowName("Filter & Combine Plots");
    fMain->MapSubwindows();


    // #### Signal/Slots ####
    searchBox->Connect("TextChanged(const char *)", "MyMainFrame", this, "FilterBySearchBox()");
    displayPathCheckBox->Connect("Clicked()", "MyMainFrame", this, "Toggle_path_display()");

    mainListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "AddToSelection(Int_t)");
    selectionListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "RemoveFromSelection(Int_t)");

    previewSelectionButton->Connect("Clicked()", "MyMainFrame", this, "PreviewSelection()");
    superimposeButton->Connect("Clicked()", "MyMainFrame", this, "Superimpose()");
    clearSelectionButton->Connect("Clicked()", "MyMainFrame", this, "ClearSelectionListbox()");

    // #### Init Window ####
    fMain->MapWindow();
    fMain->MoveResize(200, 300, 450, 600);
    InitAll();
}

MyMainFrame::~MyMainFrame() {
    fMain->Cleanup();
    delete fMain;
}

string MyMainFrame::LoadFileFromDialog()
{
    TGFileInfo file_info_;
    const char *filetypes[] = {"ROOT files", "*.root", 0, 0};
    file_info_.fFileTypes = filetypes;
    loadDialog = new TGFileDialog(gClient->GetDefaultRoot(), fMain, kFDOpen, &file_info_);

    return file_info_.fFilename;
}

void MyMainFrame::InitAll(){

//    string file_name = LoadFileFromDialog();
    string file_name = "/home/fil/projects/PR/root/GuiPlotTool/DQM_V0001_SiStrip_R000283283.root";
    file = TFile::Open(file_name.c_str());

    file ?  cout << "[ OK ]" : cout << "[FAIL]";
    cout << " Opening File" << endl;

    string baseDirStr = "DQMData";
    baseDir = (TDirectory *)file->Get(baseDirStr.c_str());

    baseDir ?  cout << "[ OK ]" : cout << "[FAIL]";
    cout << " Entering Directory" << endl;

    string currentPath = file_name + ":/" + string(baseDirStr);

    currdirLabel->SetText(currentPath.c_str());
    searchBox->SetText("");

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
            HistogramInfo value_entry(h, path);

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

void MyMainFrame::AddToSelection(Int_t id) {
    HistogramInfo val = table.find(id)->second;
    selection.insert(make_pair(id, val));
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::RemoveFromSelection(Int_t id) {
    selection.erase(id);
    DisplayMapInListBox(selection, selectionListBox);
}

void MyMainFrame::ClearSelectionListbox() {
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
