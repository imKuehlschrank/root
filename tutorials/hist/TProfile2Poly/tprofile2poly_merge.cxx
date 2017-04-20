// Brief: Check if vector based merge and TList based merge are compatible

#include <iostream>
using namespace std;

void tprofile2poly_merge()
{
   TCanvas *c1 = new TCanvas("c1", "multipads", 900, 700);
   int NUM_LS = 3;

   auto *whole = new TH2Poly();
   auto *whole_avg = new TProfile2Poly();
   auto *TP2D_avg = new TProfile2D("", "", 16, -4, 4, 16, -4, 4, 0, 100);

   auto *abso = new TH2Poly[NUM_LS];
   auto *avgs = new TProfile2Poly[NUM_LS];

   auto *empty = new TProfile2Poly();
   auto *empty2 = new TProfile2Poly();

   float minx = -4;
   float maxx = 4;
   float miny = -4;
   float maxy = 4;
   float binsz = 0.5;

   for (float i = minx; i < maxx; i += binsz) {
      for (float j = miny; j < maxy; j += binsz) {

         whole_avg->AddBin(i, j, i + binsz, j + binsz);
         whole->AddBin(i, j, i + binsz, j + binsz);
         empty->AddBin(i, j, i + binsz, j + binsz);
         empty2->AddBin(i, j, i + binsz, j + binsz);

         for (int kk = 0; kk <= NUM_LS - 1; ++kk) {
            avgs[kk].AddBin(i, j, i + binsz, j + binsz);
            abso[kk].AddBin(i, j, i + binsz, j + binsz);
         }
      }
   }

   TRandom ran;
   c1->Divide(3, 4);
   Double_t ii = 0;

   // Fill histograms events
   for (int i = 0; i <= NUM_LS - 1; ++i) {
      for (int j = 0; j < 10000; ++j) {
         Double_t r1 = ran.Gaus(0, 2);
         Double_t r2 = ran.Gaus(0, 4);

         Double_t rok = ran.Gaus(20, 2);
         Double_t rbad1 = ran.Gaus(-10, 5);

         Double_t val = rok;
         ii = double(i) * 0.5;

         if (r2 > 0.5 && r2 < 1 && r1 > 1 + ii && r1 < 1.5 + ii) val = rok - rbad1;

         whole->Fill(r1, r2);
         whole_avg->Fill(r1, r2, val);
         abso[i].Fill(r1, r2);
         avgs[i].Fill(r1, r2, val);
         TP2D_avg->Fill(r1, r2, val);

         if (j % 5000 == 0) { // so that your computer doesn't die a horrible death by update()
            c1->cd(8);
            whole_avg->SetTitle("Running Average");
            whole_avg->Draw("COLZ TEXT");
            c1->Update();
         }
      }

      string title;
      c1->cd(i + 1);
      title = " avg charge in LumiSec " + to_string(i);
      avgs[i].SetTitle(title.c_str());
      avgs[i].Draw("COLZ TEXT");
      c1->Update();

      c1->cd(i + 3 + 1);
      title = " abs hits in LumiSec " + to_string(i);
      abso[i].SetTitle(title.c_str());
      abso[i].Draw("COLZ TEXT");
      c1->Update();
   }

   c1->cd(9);
   whole->SetTitle("total hits");
   whole->Draw("COLZ TEXT");

   c1->cd(7);
   vector<TProfile2Poly *> list;
   list.push_back(&avgs[0]);
   list.push_back(&avgs[1]);
   list.push_back(&avgs[2]);
   empty->Merge(list);

   empty->SetTitle("merge avg0, avg1, avg2 Manually w/ Vector");
   empty->Draw("COLZ TEXT");

   TList li;
   li.Add(&avgs[0]);
   li.Add(&avgs[1]);
   li.Add(&avgs[2]);

   empty2->Merge(&li);
   c1->cd(10);
   empty2->SetTitle("merge avg0, avg1, avg2 Manually w/ TList");
   empty2->Draw("COLZ TEXT");

   c1->cd(11);
   TP2D_avg->SetTitle("TProfile2D Average");
   TP2D_avg->Draw("COLZ TEXT");
}
