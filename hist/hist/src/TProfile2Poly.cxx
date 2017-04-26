#include "TProfile2Poly.h"
#include "TProfileHelper.h"

#include "TMultiGraph.h"
#include "TGraph.h"
#include "TClass.h"
#include "TList.h"
#include "TMath.h"

#include <cassert>
#include <cmath>

ClassImp(TProfile2Poly)

// -------------- TProfile2PolyBin  --------------

TProfile2PolyBin::TProfile2PolyBin()
{
   fSumw = 0;
   fSumvw = 0;
   fSumw2 = 0;
   fSumwv2 = 0;
   fError = 0;
   fAverage = 0;
}

TProfile2PolyBin::TProfile2PolyBin(TObject *poly, Int_t bin_number) : TH2PolyBin(poly, bin_number)
{
   fSumw = 0;
   fSumvw = 0;
   fSumw2 = 0;
   fSumwv2 = 0;
   fError = 0;
   fAverage = 0;
}

void TProfile2PolyBin::Update()
{
   UpdateAverage();
   UpdateError();
   SetChanged(true);
}

void TProfile2PolyBin::UpdateAverage()
{
   if (fSumw != 0) fAverage = fSumvw / fSumw;
}

void TProfile2PolyBin::UpdateError()
{
   if (fSumw != 0) fError = std::sqrt((fSumwv2 / fSumw) - (fAverage * fAverage));
   if (fErrorMode == kERRORMEAN) fError /= std::sqrt(GetEffectiveEntries());
}

void TProfile2PolyBin::ClearStats()
{
   fSumw = 0;
   fSumvw = 0;
   fSumw2 = 0;
   fSumwv2 = 0;
   fError = 0;
   fAverage = 0;
}

void TProfile2PolyBin::Fill(Double_t value, Double_t weight)
{
   fSumw += weight;
   fSumvw += value * weight;
   fSumw2 += weight * weight;
   fSumwv2 += weight * value * value;
   this->Update();
}

// -------------- TProfile2Poly  --------------

TProfile2Poly::TProfile2Poly(const char *name, const char *title, Double_t xlow, Double_t xup, Double_t ylow,
                             Double_t yup)
   : TH2Poly(name, title, xlow, xup, ylow, yup)
{
}

TProfile2Poly::TProfile2Poly(const char *name, const char *title, Int_t nX, Double_t xlow, Double_t xup, Int_t nY,
                             Double_t ylow, Double_t yup)
   : TH2Poly(name, title, nX, xlow, xup, nY, ylow, yup)
{
}

TProfile2PolyBin *TProfile2Poly::CreateBin(TObject *poly)
{
   if (!poly) return 0;

   if (fBins == 0) {
      fBins = new TList();
      fBins->SetOwner();
   }

   fNcells++;
   Int_t ibin = fNcells - kNOverflow;
   return new TProfile2PolyBin(poly, ibin);
}

Int_t TProfile2Poly::Fill(Double_t xcoord, Double_t ycoord, Double_t value)
{
   return Fill(xcoord, ycoord, value, 1);
}

Int_t TProfile2Poly::Fill(Double_t xcoord, Double_t ycoord, Double_t value, Double_t weight)
{
   // Find region in which the hit occured
   Int_t tmp = GetOverflowRegionFromCoordinates(xcoord, ycoord);
   Int_t overflow_idx = OverflowIdxToArrayIdx(tmp);
   regions[overflow_idx].Fill(value, weight);

   // Find the cell to which (x,y) coordinates belong to
   Int_t n = (Int_t)(floor((xcoord - fXaxis.GetXmin()) / fStepX));
   Int_t m = (Int_t)(floor((ycoord - fYaxis.GetXmin()) / fStepY));

   // Make sure the array indices are correct.
   if (n >= fCellX) n = fCellX - 1;
   if (m >= fCellY) m = fCellY - 1;
   if (n < 0) n = 0;
   if (m < 0) m = 0;

   // ------------ Update global (per histo) statistics
   fTsumw += weight;
   fTsumw2 += weight * weight;
   fTsumwx += weight * xcoord;
   fTsumwx2 += weight * xcoord * xcoord;
   fTsumwy += weight * ycoord;
   fTsumwy2 += weight * ycoord * ycoord;
   fTsumwxy += weight * xcoord * ycoord;
   fTsumwz += weight * value;
   fTsumwz2 += weight * value * value;

   // ------------ Update local (per bin) statistics
   TProfile2PolyBin *bin;
   TIter next(&fCells[n + fCellX * m]);
   TObject *obj;
   while ((obj = next())) {
      bin = (TProfile2PolyBin *)obj;
      if (bin->IsInside(xcoord, ycoord)) {
         fEntries++;
         bin->Fill(value, weight);
         bin->Update();
         bin->SetContent(bin->fAverage);
      }
   }

   return tmp;
}

Long64_t TProfile2Poly::Merge(TCollection *in)
{
   Int_t size = in->GetSize();

   std::vector<TProfile2Poly *> list;
   list.reserve(size);

   for (int i = 0; i < size; i++) {
      list.push_back((TProfile2Poly *)((TList *)in)->At(i));
   }
   return this->Merge(list);
}

Long64_t TProfile2Poly::Merge(std::vector<TProfile2Poly *> list)
{
   if (list.size() == 0) {
      std::cout << "[FAIL] TProfile2Poly::Merge: No objects to be merged " << std::endl;
      return -1;
   }

   // ------------ Check that bin numbers of TP2P's to be merged are equal
   std::set<Int_t> numBinUnique;
   for (const auto &histo : list) {
      numBinUnique.insert(histo->fBins->GetSize());
   }
   if (numBinUnique.size() != 1) {
      std::cout << "[FAIL] TProfile2Poly::Merge: Bin numbers of TProfile2Polys to be merged differ!" << std::endl;
      return -1;
   }
   Int_t nbins = *numBinUnique.begin();

   // ------------ Update global (per histo) statistics
   for (const auto &histo : list) {
      this->fEntries += histo->fEntries;
      this->fTsumw += histo->fTsumw;
      this->fTsumw2 += histo->fTsumw2;
      this->fTsumwx += histo->fTsumwx;
      this->fTsumwx2 += histo->fTsumwx2;
      this->fTsumwy += histo->fTsumwy;
      this->fTsumwy2 += histo->fTsumwy2;
      this->fTsumwxy += histo->fTsumwxy;
      this->fTsumwz += histo->fTsumwz;
      this->fTsumwz2 += histo->fTsumwz2;

      // Merge overflow bins
      for (Int_t i = 0; i < kNOverflow; ++i) {
         this->regions[i].fSumw += histo->GetOverflowContent(i);
      }
   }

   // ------------ Update local (per bin) statistics
   TProfile2PolyBin *dst = nullptr;
   TProfile2PolyBin *src = nullptr;
   for (Int_t i = 0; i < nbins; i++) {
      dst = (TProfile2PolyBin *)fBins->At(i);

      Double_t sumv_acc = 0;
      Double_t sumw_acc = 0;
      Double_t sumvw_acc = 0;
      Double_t sumv2_acc = 0;
      Double_t sumwv2_acc = 0;

      // accumulate values of interest in the input vector
      for (const auto &e : list) {
         src = (TProfile2PolyBin *)e->fBins->At(i);
         sumv_acc += src->fSumv;
         sumw_acc += src->fSumw;
         sumvw_acc += src->fSumvw;
         sumv2_acc += src->fSumw2;
         sumwv2_acc += src->fSumwv2;
      }

      // add values of accumulation to existing histogram
      dst->fSumw += sumw_acc;
      dst->fSumvw += sumvw_acc;
      dst->fSumw2 += sumv2_acc;
      dst->fSumwv2 += sumwv2_acc;

      // update averages, errors
      dst->Update();
   }
   this->SetContentToAverage();
   return 1;
}

void TProfile2Poly::SetContentToAverage()
{
   Int_t nbins = fBins->GetSize();
   for (Int_t i = 0; i < nbins; i++) {
      TProfile2PolyBin *bin = (TProfile2PolyBin *)fBins->At(i);
      bin->Update();
      bin->SetContent(bin->fAverage);
   }
}

void TProfile2Poly::SetContentToError()
{
   Int_t nbins = fBins->GetSize();
   for (Int_t i = 0; i < nbins; i++) {
      TProfile2PolyBin *bin = (TProfile2PolyBin *)fBins->At(i);
      bin->Update();
      bin->SetContent(bin->fError);
   }
}

Double_t TProfile2Poly::GetBinEffectiveEntries(Int_t binnr)
{
    //todo
    if (bin > GetNumberOfBins() || bin == 0 || bin < -kNOverflow) return 0;
    if (bin<0) return fOverflow[-bin - 1];
    return ((TH2PolyBin*) fBins->At(bin-1))->GetContent();
}

void TProfile2Poly::printOverflowRegions()
{
   Double_t total = 0;
   Double_t cont = 0;
   for (Int_t i = 0; i < kNOverflow; ++i) {
      cont = GetOverflowContent(i);
      total += cont;
      std::cout << "\t" << cont << "\t";
      if ((i + 1) % 3 == 0) std::cout << std::endl;
   }

   std::cout << "Total: " << total << std::endl;
}

void TProfile2Poly::Reset(Option_t *opt)
{
   TIter next(fBins);
   TObject *obj;
   TProfile2PolyBin *bin;

   // Clears bin contents
   while ((obj = next())) {
      bin = (TProfile2PolyBin *)obj;
      bin->ClearContent();
      bin->ClearStats();
   }
   TH2::Reset(opt);
}

Int_t TProfile2Poly::GetOverflowRegionFromCoordinates(Double_t x, Double_t y)
{
   // The overflow regions are calculated by considering x, y coordinates.
   // The Middle bin at -5 contains all the TProfile2Poly bins.
   //
   //           -0 -1 -2
   //           ________
   //    -1:   |__|__|__|
   //    -4:   |__|__|__|
   //    -7:   |__|__|__|

   Int_t region = 0;

   if (fNcells <= kNOverflow) return 0;

   // --- y offset
   if (y > fYaxis.GetXmax())
      region += -1;
   else if (y > fYaxis.GetXmin())
      region += -4;
   else
      region += -7;

   // --- x offset
   if (x > fXaxis.GetXmax())
      region += -2;
   else if (x > fXaxis.GetXmin())
      region += -1;
   else
      region += 0;

   return region;
}

void TProfile2Poly::SetErrorOption(TProfile2Poly::EErrorProfileType type)
{
  fErrorMode = type;

  TIter next(fBins);
  TObject *obj;
  TProfile2PolyBin *bin;
  while ((obj = next())) {
     bin = (TProfile2PolyBin *)obj;
     bin->SetErrorOption(type);
  }
}
