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

    void PreviewSelection();
    void Superimpose();
    void ClearSelectionListbox();

    string LoadFileFromDialog();

    void ToggleEnableRenameTextbox();
    void UpdateDistplayListboxes();


    void HandleMenu(Int_t a);

private:
    // GUI elements
    TGMenuBar* fMenuBar;
    TGPopupMenu* fMenuFile;

    TGMainFrame* fMain;
    TGLabel*     currdirLabel;
    TGTextEntry* searchBox;
    TGTextEntry* renameTextbox;

    TGListBox*   mainListBox;
    TGListBox*   selectionListBox;

    TGCheckButton* displayPathCheckBox;
    TGCheckButton* renameCheckbox;
    TGCheckButton* statsCheckBox;
    TGCheckButton* tdrCheckBox;


    TCanvas* previewCanvas;
    TCanvas* resultCanvas;

    // File Stuff
    TFile*      file;
    TDirectory* baseDir;


    map<Int_t, HistogramInfo> table;
    map<Int_t, HistogramInfo> selection;

    TGFileDialog* loadDialog;

    Int_t freeId = 0;
    Int_t GetNextFreeId() { return freeId++; }

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
    fMenuFile->Connect("Activated(Int_t)", "MyMainFrame", this, "HandleMenu(Int_t)");
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
    mainListBox = new TGListBox(fMain, -1, kSunkenFrame);
    selectionListBox = new TGListBox(fMain, -1, kSunkenFrame);

    // --- All Controls
    TGHorizontalFrame* controlFrame = new TGHorizontalFrame(fMain, 200, 40);

        // ------- Buttons
    TGVerticalFrame* controlFrameButtons = new TGVerticalFrame(controlFrame, 200, 40);

    TGTextButton* previewSelectionButton = new TGTextButton(controlFrameButtons, "&Preview List");
    TGTextButton* superimposeButton      = new TGTextButton(controlFrameButtons, "&Superimpose");
    TGTextButton* clearSelectionButton   = new TGTextButton(controlFrameButtons, "&Clear Selection");

    controlFrameButtons->AddFrame(clearSelectionButton,   new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameButtons->AddFrame(previewSelectionButton, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameButtons->AddFrame(superimposeButton,      new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Checkboxes
    TGVerticalFrame* controlFrameCheckboxes = new TGVerticalFrame(controlFrame, 200, 80);

    tdrCheckBox   = new TGCheckButton(controlFrameCheckboxes, "Pub. Style");
    statsCheckBox = new TGCheckButton(controlFrameCheckboxes, "Show Stats");
    TGCheckButton* cb1 = new TGCheckButton(controlFrameCheckboxes,"Use Colors");
    TGCheckButton* cb2 = new TGCheckButton(controlFrameCheckboxes,"Add Legend ");

    controlFrameCheckboxes->AddFrame(tdrCheckBox,   new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(statsCheckBox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(cb1, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameCheckboxes->AddFrame(cb2, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // ------- Rename
    TGVerticalFrame* controlFrameRename = new TGVerticalFrame(controlFrame, 200, 40);

    renameCheckbox = new TGCheckButton(controlFrameRename,"Use Custom Title");
    renameTextbox  = new TGTextEntry(controlFrameRename);

    controlFrameRename->AddFrame(renameCheckbox, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));
    controlFrameRename->AddFrame(renameTextbox,  new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 3, 4));

        // --- Add to All Controls
    controlFrame->AddFrame(controlFrameButtons,    new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameCheckboxes, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    controlFrame->AddFrame(controlFrameRename,     new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));

    // ---- Main Frame, Adding all the individual frames in the appropriate order top to bottom

    fMain->AddFrame(fMenuBar,           new TGLayoutHints(kLHintsLeft ,2,2,2,2));
    fMain->AddFrame(quicksearchFrame,   new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
    fMain->AddFrame(currdirLabel,       new TGLayoutHints(kLHintsLeft | kLHintsExpandX ,2,2,2,2));
    fMain->AddFrame(mainListBox,        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 6));
    fMain->AddFrame(selectionListBox,   new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 6));
    fMain->AddFrame(controlFrame,       new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));

    fMain->SetWindowName("Filter & Combine Plots");
    fMain->MapSubwindows();


    // #### Signal/Slots ####
    searchBox->Connect("TextChanged(const char *)", "MyMainFrame", this, "FilterBySearchBox()");
    displayPathCheckBox->Connect("Clicked()", "MyMainFrame", this, "UpdateDistplayListboxes()");

    mainListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "AddToSelection(Int_t)");
    selectionListBox->Connect("DoubleClicked(Int_t)", "MyMainFrame", this, "RemoveFromSelection(Int_t)");

    previewSelectionButton->Connect("Clicked()", "MyMainFrame", this, "PreviewSelection()");
    superimposeButton->Connect("Clicked()", "MyMainFrame", this, "Superimpose()");
    clearSelectionButton->Connect("Clicked()", "MyMainFrame", this, "ClearSelectionListbox()");

    renameCheckbox->Connect("Clicked()", "MyMainFrame", this, "ToggleEnableRenameTextbox()");

    // #### Init Window ####
    fMain->MapWindow();
    fMain->MoveResize(100, 100, 600, 700);
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


void MyMainFrame::InitAll() {

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

    // Set up all the GUI elements
    currdirLabel->SetText(currentPath.c_str());
    searchBox->SetText("");
    renameTextbox->SetEnabled(false);

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
    for (auto &elem : selection) {
        previewCanvas->cd(i++);
        elem.second.GetObj()->Draw();
    }
}

void MyMainFrame::Superimpose() {
    resultCanvas = new TCanvas("Superimpose Canvas", "", 800, 400);
    resultCanvas->cd();

    bool firstElem = true;
    string title;
    TH1* out;
    for(auto& elem : selection){

        if(firstElem){
            out = (TH1*)elem.second.GetObj()->Clone();

            (renameCheckbox->IsOn()) ? title=renameTextbox->GetText(): title=out->GetTitle();
            out->SetTitle(title.c_str());

            out->SetStats(statsCheckBox->IsOn());  // FIXME: only shows the first statbox
            out->Draw("same");
            firstElem=false;

        } else {
            ((TH1*)elem.second.GetObj())->SetStats(statsCheckBox->IsOn()); // FIXME: only shows the first statbox
            elem.second.GetObj()->Draw("same");

        }
    }
}

void GuiPlotTool() {
    new MyMainFrame(gClient->GetRoot(), 200, 200);
}
