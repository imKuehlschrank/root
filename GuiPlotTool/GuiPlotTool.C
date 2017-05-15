#include <TGClient.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <RQ_OBJECT.h>
#include "TStyle.h"


// To search for loading SiStrip files, for nice visuals:
//OnTrack__TID__PLUS__ring__
//InPixel

using namespace std;

enum EMyMessageTypes {
   M_FILE_OPEN,
   M_FILE_EXIT
};

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

    void ClearSelectionListbox();
    void PreviewSelection();
    void Superimpose();
    void MergeSelection();

    string LoadFileFromDialog();

    void ToggleEnableRenameTextbox();
    void ToggleXMaxTextbox();
    void ToggleYMaxTextbox();

    void UpdateDistplayListboxes();
    void HandleMenu(Int_t a);

    void setTDRStyle();

    void SetCheckboxOptions(TH1* elem);
    void CalcSuperimpose(vector<TH1*>& plots, vector<TPaveStats*>& statboxes);
    void DrawPlots(vector<TH1*>& plots, vector<TPaveStats*>& statboxes, string option="");

private:
    // GUI elements
    TGMenuBar*    fMenuBar;
    TGPopupMenu*  fMenuFile;
    TGFileDialog* loadDialog;

    TGMainFrame* fMain;
    TGLabel*     currdirLabel;

    TGTextEntry* searchBox;
    TGTextEntry* renameTextbox;

    TGNumberEntryField* xminNumbertextbox;
    TGNumberEntryField* xmaxNumbertextbox;
    TGNumberEntryField* yminNumbertextbox;
    TGNumberEntryField* ymaxNumbertextbox;

    TGListBox*   mainListBox;
    TGListBox*   selectionListBox;

    TGCheckButton* displayPathCheckBox;
    TGCheckButton* tdrstyleCheckBox;
    TGCheckButton* statsCheckBox;
    TGCheckButton* renameCheckbox;
    TGCheckButton* xRangeCheckbox;
    TGCheckButton* yRangeCheckbox;

    TCanvas* previewCanvas = nullptr;
    TCanvas* resultCanvas = nullptr;

    // File Stuff
    TFile*      file;
    TDirectory* baseDir;

    // Model
    map<Int_t, HistogramInfo> table;
    map<Int_t, HistogramInfo> selection;

    Int_t freeId = 0;
    Int_t GetNextFreeId() { return freeId++; }

    void ResetGuiElements();
    void InitAll();
};


MyMainFrame::MyMainFrame(const TGWindow *p, UInt_t w, UInt_t h) {
    fMain = new TGMainFrame(p, w, h);

    // #### DESIGN ####

    // ---- Menu Bar
    fMenuBar = new TGMenuBar(fMain, 35, 50, kHorizontalFrame);

    fMenuFile = new TGPopupMenu(gClient->GetRoot());
    fMenuFile->AddEntry(" &Open...\tCtrl+O", M_FILE_OPEN, 0, gClient->GetPicture("bld_open.png"));
    fMenuFile->AddSeparator();
    fMenuFile->AddEntry(" E&xit\tCtrl+Q", M_FILE_EXIT, 0, gClient->GetPicture("bld_exit.png"));
    fMenuBar->AddPopup("&File", fMenuFile, new TGLayoutHints(kLHintsLeft, 0, 4, 0, 0));

    // ---- Top label
    currdirLabel = new TGLabel(fMain,"");
    currdirLabel->MoveResize(0,0,500,16);

    // ---- Quicksearch Frame
    TGGroupFrame* quicksearchFrame = new TGGroupFrame(fMain,"Quicksearch");

    searchBox = new TGTextEntry(quicksearchFrame);
    displayPathCheckBox = new TGCheckButton(quicksearchFrame,"Display path");

    quicksearchFrame->AddFrame(searchBox,           new TGLayoutHints(kLHintsExpandX ,2,2,2,2));
    quicksearchFrame->AddFrame(displayPathCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

    // ---- Listboxes
    TGVerticalFrame* listboxesFrame = new TGVerticalFrame(fMain, 200, 40);

    TGHorizontalFrame* mainListboxFrame = new TGHorizontalFrame(listboxesFrame, 10, 250, kFixedHeight);
    TGHorizontalFrame* selectionListboxFrame = new TGHorizontalFrame(listboxesFrame);

    mainListBox      = new TGListBox(mainListboxFrame, -1, kSunkenFrame);
    selectionListBox = new TGListBox(selectionListboxFrame, -1, kSunkenFrame);

    mainListboxFrame->AddFrame(mainListBox,          new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
    selectionListboxFrame->AddFrame(selectionListBox,new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

    TGHSplitter *hsplitter = new TGHSplitter(listboxesFrame,2,2);
    hsplitter->SetFrame(mainListboxFrame, kTRUE);

    listboxesFrame->AddFrame(mainListboxFrame,      new TGLayoutHints(kLHintsExpandX, 5, 5, 3, 4));
    listboxesFrame->AddFrame(hsplitter,             new TGLayoutHints(kLHintsTop | kLHintsExpandX));
    listboxesFrame->AddFrame(selectionListboxFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY , 5, 5, 3, 4));


    // --- All Controls
    TGHorizontalFrame* controlFrame = new TGHorizontalFrame(fMain, 200, 40);

        // ------- Selection Buttons
    TGVerticalFrame* selectionControlFrameButtons = new TGVerticalFrame(controlFrame, 200, 40);

    TGTextButton* clearSelectionButton   = new TGTextButton(selectionControlFrameButtons, "&Clear List");
    TGTextButton* previewSelectionButton = new TGTextButton(selectionControlFrameButtons, "&Preview List");

    selectionControlFrameButtons->AddFrame(clearSelectionButton,   new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    selectionControlFrameButtons->AddFrame(previewSelectionButton, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Checkboxes
    TGVerticalFrame* controlFrameCheckboxes = new TGVerticalFrame(controlFrame, 200, 80);

    tdrstyleCheckBox = new TGCheckButton(controlFrameCheckboxes, "Pub. Style");
    statsCheckBox    = new TGCheckButton(controlFrameCheckboxes, "Show Stats");

    controlFrameCheckboxes->AddFrame(tdrstyleCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(statsCheckBox,    new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Output Buttons
    TGVerticalFrame* outputControlFrameButtons = new TGVerticalFrame(controlFrame, 200, 40);

    TGTextButton* mergeSelectionButton    = new TGTextButton(outputControlFrameButtons, "&Merge Only");
    TGTextButton* superimposeButton       = new TGTextButton(outputControlFrameButtons, "&Superimpose");
    TGVertical3DLine* outputseperatorLine = new TGVertical3DLine(controlFrame);

    outputControlFrameButtons->AddFrame(mergeSelectionButton, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    outputControlFrameButtons->AddFrame(superimposeButton,    new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Custom
    TGVerticalFrame* controlFrameCustom = new TGVerticalFrame(controlFrame, 200, 40);

    renameCheckbox = new TGCheckButton(controlFrameCustom,"Use Custom Title");
    renameTextbox  = new TGTextEntry(controlFrameCustom);

            // X Axis
    TGHorizontalFrame* controlFrameXRange = new TGHorizontalFrame(controlFrameCustom, 200, 40);

    xRangeCheckbox    = new TGCheckButton(controlFrameCustom,"Use Custom X Range");
    xminNumbertextbox = new TGNumberEntryField(controlFrameXRange);
    xmaxNumbertextbox = new TGNumberEntryField(controlFrameXRange);

    controlFrameXRange->AddFrame(xminNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameXRange->AddFrame(xmaxNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

            // Y Axis
    TGHorizontalFrame* controlFrameYRange = new TGHorizontalFrame(controlFrameCustom, 200, 40);

    yRangeCheckbox    = new TGCheckButton(controlFrameCustom,"Use Custom Y Range");
    yminNumbertextbox = new TGNumberEntryField(controlFrameYRange);
    ymaxNumbertextbox = new TGNumberEntryField(controlFrameYRange);

    controlFrameYRange->AddFrame(yminNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameYRange->AddFrame(ymaxNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Add to Custom
    controlFrameCustom->AddFrame(renameCheckbox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCustom->AddFrame(renameTextbox,      new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

    controlFrameCustom->AddFrame(xRangeCheckbox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCustom->AddFrame(controlFrameXRange, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

    controlFrameCustom->AddFrame(yRangeCheckbox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCustom->AddFrame(controlFrameYRange, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // --- Add to All Controls
    controlFrame->AddFrame(selectionControlFrameButtons, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameCheckboxes,       new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameCustom,           new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(outputseperatorLine,          new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
    controlFrame->AddFrame(outputControlFrameButtons,    new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    // ---- Main Frame, Adding all the individual frames in the appropriate order top to bottom
    fMain->AddFrame(fMenuBar,           new TGLayoutHints(kLHintsLeft ,2,2,2,2));
    fMain->AddFrame(quicksearchFrame,   new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
    fMain->AddFrame(currdirLabel,       new TGLayoutHints(kLHintsLeft | kLHintsExpandX ,2,2,2,2));
    fMain->AddFrame(listboxesFrame,     new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 6));
    fMain->AddFrame(controlFrame,       new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));

    fMain->SetWindowName("Filter & Combine Plots");
    fMain->MapSubwindows();

    // #### Signal/Slots ####

    fMenuFile->Connect("Activated(Int_t)", "MyMainFrame", this, "HandleMenu(Int_t)");

    searchBox->Connect("TextChanged(const char *)", "MyMainFrame", this, "FilterBySearchBox()");
    displayPathCheckBox->Connect("Clicked()", "MyMainFrame", this, "UpdateDistplayListboxes()");

    mainListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "AddToSelection(Int_t)");
    selectionListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "RemoveFromSelection(Int_t)");

    clearSelectionButton->Connect("Clicked()", "MyMainFrame", this, "ClearSelectionListbox()");
    previewSelectionButton->Connect("Clicked()", "MyMainFrame", this, "PreviewSelection()");
    superimposeButton->Connect("Clicked()", "MyMainFrame", this, "Superimpose()");
    mergeSelectionButton->Connect("Clicked()", "MyMainFrame", this, "MergeSelection()");

    renameCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleEnableRenameTextbox()");
    xRangeCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleXMaxTextbox()");
    yRangeCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleYMaxTextbox()");

    // #### Init Window ####

    fMain->MapWindow();
    fMain->MoveResize(100, 100, 600, 700);
    ResetGuiElements();
}

MyMainFrame::~MyMainFrame() {
    fMain->Cleanup();
    delete fMain;
}

void MyMainFrame::HandleMenu(Int_t menu_id)
{
    switch (menu_id) {
    case M_FILE_OPEN:
        cout << "[ OK ] Open File Dialog" << endl;
        InitAll();
        break;

    case M_FILE_EXIT:
        gApplication->Terminate(0);
        break;
    }
}

string MyMainFrame::LoadFileFromDialog()
{
    TGFileInfo file_info_;
    const char *filetypes[] = {"ROOT files", "*.root", 0, 0};
    file_info_.fFileTypes = filetypes;
    loadDialog = new TGFileDialog(gClient->GetDefaultRoot(), fMain, kFDOpen, &file_info_);

    return file_info_.fFilename;
}

void MyMainFrame::ToggleEnableRenameTextbox() {
    renameTextbox->SetEnabled(renameCheckbox->IsDown());
}

void MyMainFrame::ToggleXMaxTextbox() {
    xminNumbertextbox->SetEnabled(xRangeCheckbox->IsDown());
    xmaxNumbertextbox->SetEnabled(xRangeCheckbox->IsDown());
}

void MyMainFrame::ToggleYMaxTextbox() {
    yminNumbertextbox->SetEnabled(yRangeCheckbox->IsDown());
    ymaxNumbertextbox->SetEnabled(yRangeCheckbox->IsDown());
}

void MyMainFrame::ResetGuiElements() {
    currdirLabel->SetText("");
    searchBox->SetText("");
    renameTextbox->SetEnabled(false);
    xminNumbertextbox->SetEnabled(false);
    xmaxNumbertextbox->SetEnabled(false);
    yminNumbertextbox->SetEnabled(false);
    ymaxNumbertextbox->SetEnabled(false);

    displayPathCheckBox->SetState(kButtonUp);
    statsCheckBox->SetState(kButtonUp);
    tdrstyleCheckBox->SetState(kButtonUp);
}

void MyMainFrame::InitAll() {
//    ResetGuiElements();

    string file_name = LoadFileFromDialog();
//    string file_name = "/home/fil/projects/PR/root/GuiPlotTool/DQM_V0001_SiStrip_R000283283.root";
    file = TFile::Open(file_name.c_str());

    file ?  cout << "[ OK ]" : cout << "[FAIL]";
    cout << " Opening File" << endl;

    string baseDirStr = "DQMData";
    baseDir = (TDirectory*)file->Get(baseDirStr.c_str());

    baseDir ?  cout << "[ OK ]" : cout << "[FAIL]";
    cout << " Entering Directory" << endl;

    string currentPath = file_name + ":/" + string(baseDirStr);
    currdirLabel->SetText(currentPath.c_str());

    // Fill internal data structes from file and display
    LoadAllPlotsFromDir(baseDir);
    DisplayMapInListBox(table, mainListBox);
}

void MyMainFrame::UpdateDistplayListboxes() {
    DisplayMapInListBox(table, mainListBox);
    DisplayMapInListBox(selection, selectionListBox);
    FilterBySearchBox();
}

void MyMainFrame::LoadAllPlotsFromDir(TDirectory *current) {
    TIter next(current->GetListOfKeys());
    TKey* key;

    while ((key = (TKey* )next())) {
        TClass* cl = gROOT->GetClass(key->GetClassName());

        if (cl->InheritsFrom("TDirectory")) {
            TDirectory* d = (TDirectory*)key->ReadObj();
            LoadAllPlotsFromDir(d);
        }

        if (cl->InheritsFrom("TH1")) {
            TH1* h = (TH1* )key->ReadObj();
            string fname = h->GetName();
            string path = string(current->GetPath()) + "/" + fname;

            Int_t key_entry = GetNextFreeId();
            HistogramInfo value_entry(h, path);

            table.insert(make_pair(key_entry, value_entry));
        }
    }
}

void MyMainFrame::DisplayMapInListBox(const map<Int_t, HistogramInfo>& m, TGListBox* listbox) {
    listbox->RemoveAll();

    string disp;
    for (auto& elem : m) {
        displayPathCheckBox->IsDown() ? disp = elem.second.GetPath() : disp = elem.second.GetName();
        listbox->AddEntry(disp.c_str(), elem.first);
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
    for (auto& elem : selection) {
        previewCanvas->cd(i++);
        elem.second.GetObj()->Draw();
    }
}

// statboxes : OUT param
void MyMainFrame::CalcSuperimpose(vector<TH1*>& plots, vector<TPaveStats*>& statboxes) {
    // collect the statboxes
    TPaveStats* tstat;
    double X1, Y1, X2, Y2;

    TCanvas* tmpCanvas = new TCanvas("a Canvas", "", 800, 400);
    tmpCanvas->cd();

    for(int i=0; i<plots.size(); i++) {

        plots[i]->Draw();
        gPad->Update();

        tstat = (TPaveStats*) plots[i]->FindObject("stats");

        if(i!=0){
            tstat->SetX1NDC(X1);
            tstat->SetX2NDC(X2);
            tstat->SetY1NDC(Y1-(Y2-Y1));
            tstat->SetY2NDC(Y1);
        }

        X1 = tstat->GetX1NDC();
        Y1 = tstat->GetY1NDC();
        X2 = tstat->GetX2NDC();
        Y2 = tstat->GetY2NDC();

        statboxes.push_back(tstat);
    }
    delete tmpCanvas;

}

void MyMainFrame::SetCheckboxOptions(TH1* elem) {
    if(xRangeCheckbox->IsOn()) {
        elem->SetAxisRange(xminNumbertextbox->GetNumber(), xmaxNumbertextbox->GetNumber(),"X");
    }

    if(yRangeCheckbox->IsOn()) {
        elem->SetAxisRange(yminNumbertextbox->GetNumber(), ymaxNumbertextbox->GetNumber(),"Y");
    }

    if(renameCheckbox->IsOn()) {
        string tmp = renameTextbox->GetText();
        elem->SetTitle(tmp.c_str());
    }

    elem->SetStats(statsCheckBox->IsOn());
}

void MyMainFrame::DrawPlots(vector<TH1*>& plots, vector<TPaveStats*>& statboxes, string option="") {

    vector<Int_t> basic_colors = { kBlue, kGreen, kCyan, kMagenta, kRed };
    vector<Int_t> colors;

    // why god why... https://root.cern.ch/doc/master/classTColor.html
    for(auto c : basic_colors) colors.push_back(c);
    for(auto c : basic_colors) colors.push_back(c+2);
    for(auto c : basic_colors) colors.push_back(c-7);
    for(auto c : basic_colors) colors.push_back(c-4);
    for(auto c : basic_colors) colors.push_back(c-9);

    resultCanvas->cd();
    Int_t idx = 0;
    for(auto& elem : plots) {
        elem->SetLineColor(colors[idx]);
        SetCheckboxOptions(elem);
        elem->Draw(option.c_str());
        idx++;
    }

    if(statsCheckBox->IsOn()) {
        idx = 0;
        for(auto& elem : statboxes) {
            elem->SetTextColor(colors[idx]);
            elem->SetLineColor(colors[idx]);
            elem->Draw(option.c_str());

            idx++;
        }
    }
}

void MyMainFrame::Superimpose() {
    if(selection.size() == 0) {
        return;
    }

    resultCanvas = new TCanvas("Superimpose Canvas", "", 800, 400);
    resultCanvas->cd();

    // never work on the originals!
    vector<TH1*> copies;
    for(auto& elem : selection){
        copies.push_back((TH1*)elem.second.GetObj()->Clone());
    }

    vector<TPaveStats*> statboxes; // out param

    CalcSuperimpose(copies, statboxes);
    DrawPlots(copies, statboxes, "same");
}

void MyMainFrame::MergeSelection() {
    if(selection.size() == 0) {
        return;
    }

    resultCanvas = new TCanvas("Merge Canvas", "", 800, 400);
    resultCanvas->cd();

    // work on copy
    vector<TH1*> copies;
    for(auto& elem : selection){
        copies.push_back((TH1*)elem.second.GetObj()->Clone());
    }

    // set options
    for(Int_t idx=1; idx<copies.size(); ++idx) {
        copies[0]->Add(copies[idx]);
    }

    SetCheckboxOptions(copies[0]);

    copies[0]->Draw();
}



void GuiPlotTool() {
    new MyMainFrame(gClient->GetRoot(), 200, 200);
}
