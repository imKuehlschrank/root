// Brief: Approximate a TProfile2D with TProfile2Poly (Honeycomb), and see if results are similar.

void tprofile2poly_tprofile2d_sim2()
{
   auto c1 = new TCanvas("c1", "", 200, 10, 700, 500);
   c1->Divide(2, 1);

   auto TP2D = new TProfile2D("", "", 100, -10, 10, 100, -10, 10, 0, 100);
   auto TP2P = new TProfile2Poly();

   TP2P->Honeycomb(-10, -10, .1, 100, 130);
   TP2P->SetName("mine");
   TP2P->SetTitle("mine");

   int value = 1;
   Float_t px, py;
   for (Int_t i = 0; i < 30000; i++) {
      gRandom->Rannor(px, py);
      value = px * px + py * py;
      TP2D->Fill(px, py, value);
      TP2P->Fill(px, py, value);
   }
   c1->cd(1);
   TP2D->Draw("COLZ");

   c1->cd(2);
   TP2P->Draw("COLZ");
}
