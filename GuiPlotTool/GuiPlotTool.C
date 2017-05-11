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
    void ToggleYMaxTextbox();

    void UpdateDistplayListboxes();
    void HandleMenu(Int_t a);

    void setTDRStyle();

private:
    // GUI elements
    TGMenuBar*    fMenuBar;
    TGPopupMenu*  fMenuFile;
    TGFileDialog* loadDialog;

    TGMainFrame* fMain;
    TGLabel*     currdirLabel;

    TGTextEntry* searchBox;
    TGTextEntry* renameTextbox;
    TGNumberEntryField* ymaxNumbertextbox;

    TGListBox*   mainListBox;
    TGListBox*   selectionListBox;

    TGCheckButton* displayPathCheckBox;
    TGCheckButton* renameCheckbox;
    TGCheckButton* ymaxCheckbox;
    TGCheckButton* statsCheckBox;
    TGCheckButton* tdrstyleCheckBox;

    TCanvas* previewCanvas;
    TCanvas* resultCanvas;

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

    mainListBox      = new TGListBox(listboxesFrame, -1, kSunkenFrame);
    selectionListBox = new TGListBox(listboxesFrame, -1, kSunkenFrame);
    TGHorizontal3DLine* listboxseperatorLine = new TGHorizontal3DLine(listboxesFrame);

    listboxesFrame->AddFrame(mainListBox,          new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 3, 4));
    listboxesFrame->AddFrame(listboxseperatorLine, new TGLayoutHints(kLHintsExpandX, 5, 5, 3, 4));
    listboxesFrame->AddFrame(selectionListBox,     new TGLayoutHints(kLHintsExpandX , 5, 5, 3, 4));
    selectionListBox->Resize(100,140);

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

    controlFrameCheckboxes->AddFrame(tdrstyleCheckBox,   new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(statsCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Output Buttons
    TGVerticalFrame* outputControlFrameButtons = new TGVerticalFrame(controlFrame, 200, 40);

    TGTextButton* mergeSelectionButton = new TGTextButton(outputControlFrameButtons, "&Merge");
    TGTextButton* superimposeButton    = new TGTextButton(outputControlFrameButtons, "&Superimpose");
    TGVertical3DLine* outputseperatorLine = new TGVertical3DLine(controlFrame);

    outputControlFrameButtons->AddFrame(mergeSelectionButton, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    outputControlFrameButtons->AddFrame(superimposeButton,    new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Custom
    TGVerticalFrame* controlFrameRename = new TGVerticalFrame(controlFrame, 200, 40);

    renameCheckbox = new TGCheckButton(controlFrameRename,"Use Custom Title");
    renameTextbox  = new TGTextEntry(controlFrameRename);

    ymaxCheckbox      = new TGCheckButton(controlFrameRename,"Use Custom YMax");
    ymaxNumbertextbox = new TGNumberEntryField(controlFrameRename);

    controlFrameRename->AddFrame(renameCheckbox,    new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameRename->AddFrame(renameTextbox,     new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameRename->AddFrame(ymaxCheckbox,      new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameRename->AddFrame(ymaxNumbertextbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // --- Add to All Controls
    controlFrame->AddFrame(selectionControlFrameButtons, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameCheckboxes,       new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameRename,           new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
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
    ymaxCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleYMaxTextbox()");

    // #### Init Window ####

    fMain->MapWindow();
    fMain->MoveResize(100, 100, 600, 700);

    ResetGuiElements();
    InitAll();
    searchBox->SetText("InPi");
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

void MyMainFrame::ToggleYMaxTextbox() {
    ymaxNumbertextbox->SetEnabled(ymaxCheckbox->IsDown());
}

void MyMainFrame::ResetGuiElements() {
    currdirLabel->SetText("");
    searchBox->SetText("");
    ymaxNumbertextbox->SetText("");
    renameTextbox->SetEnabled(false);
    ymaxNumbertextbox->SetEnabled(false);

    displayPathCheckBox->SetState(kButtonUp);
    statsCheckBox->SetState(kButtonUp);
    tdrstyleCheckBox->SetState(kButtonUp);
}

void MyMainFrame::InitAll() {
    ResetGuiElements(); // TODO: What is the behaviour that we want??

//    string file_name = LoadFileFromDialog(); // FIXME put back
    string file_name = "/home/fil/projects/PR/root/GuiPlotTool/DQM_V0001_SiStrip_R000283283.root";
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

// superimpoes that uses only Draw("SAME")
void MyMainFrame::Superimpose() {
    resultCanvas = new TCanvas("Superimpose Canvas", "", 800, 400);
    resultCanvas->cd();

    bool firstElem = true;
    string title;
    TH1* out;
    for(auto& elem : selection){

        if(firstElem){
            out = (TH1*)elem.second.GetObj()->Clone();


            if(ymaxCheckbox->IsOn()) {
                out->SetMaximum(ymaxNumbertextbox->GetNumber());
            }

            if(renameCheckbox->IsOn()) {
                string tmp = renameTextbox->GetText();
                out->SetTitle(tmp.c_str());
            }

            out->SetStats(statsCheckBox->IsOn());  // FIXME: only shows the first statbox
            out->Draw("same");
            firstElem=false;

        } else {
            ((TH1*)elem.second.GetObj())->SetStats(statsCheckBox->IsOn()); // FIXME: only shows the first statbox

            int randNum = rand()%(40-0 + 1);

            ((TH1*)elem.second.GetObj())->SetLineColor(randNum);
            elem.second.GetObj()->Draw("same");

        }
    }
}

//void MyMainFrame::Superimpose() {
//    TCanvas* c1 = new TCanvas("Superimpose Canvas", "", 800, 400);

//    bool firstElem = true;
//    TH1 *h1;
//    TH1 *h2;


//    TPad *pad1 = new TPad("pad1","",0,0,1,1);
//    TPad *pad2 = new TPad("pad2","",0,0,1,1);

//    pad2->SetFillStyle(4000); //will be transparent


//    for(auto& elem : selection){

//        if(firstElem){
//            h1 = (TH1*)elem.second.GetObj();
//            firstElem=false;

//        } else {
//            h2 = (TH1*)elem.second.GetObj();
//        }
//    }


//    h1->Draw();
//    pad1->Update(); //this will force the generation of the "stats" box
//    TPaveStats *ps1 = (TPaveStats*)h1->GetListOfFunctions()->FindObject("stats");
//    ps1->SetX1NDC(0.4); ps1->SetX2NDC(0.6);
//    pad1->Modified();
//    c1->cd();


//    h1->SetMaximum(4000.);   // along
//    h2->SetMaximum(4000.);   // along



//    Double_t ymin = 0;
//    Double_t ymax = 2000;
//    Double_t dy = (ymax-ymin)/0.8; //10 per cent margins top and bottom
//    Double_t xmin = -3;
//    Double_t xmax = 3;
//    Double_t dx = (xmax-xmin)/0.8; //10 per cent margins left and right
//    pad2->Range(xmin-0.1*dx,ymin-0.1*dy,xmax+0.1*dx,ymax+0.1*dy);
//    pad2->Draw();
//    pad2->cd();
//    h2->SetLineColor(kRed);
//    h2->Draw("same");
//    pad2->Update();
//    TPaveStats *ps2 = (TPaveStats*)h2->GetListOfFunctions()->FindObject("stats");
//    ps2->SetX1NDC(0.65); ps2->SetX2NDC(0.85);
//    ps2->SetTextColor(kRed);
//}


void MyMainFrame::MergeSelection() {
    resultCanvas = new TCanvas("Merge Canvas", "", 800, 400);
    resultCanvas->cd();

    bool firstElem = true;
    string title;
    TH1* out;

    for(auto& elem : selection) {

        if(firstElem) {
            out = (TH1*)elem.second.GetObj()->Clone();

            (renameCheckbox->IsOn()) ? title=renameTextbox->GetText() : title=out->GetTitle();
            out->SetTitle(title.c_str());

            firstElem = false;
        } else {
            out->Add((TH1*)elem.second.GetObj());
        }
    }
    out->Draw();
}



void GuiPlotTool() {
    new MyMainFrame(gClient->GetRoot(), 200, 200);
}
