#include <fun4all/Fun4AllServer.h>

#include <calotrigger/TriggerRunInfoReco.h>
#include <globalvertex/GlobalVertexReco.h>

#include <kfparticleqa/QAKFParticle.h>
#include <kfparticleqa/QAKFParticleTrackPtAsymmetry.h>

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wundefined-internal"

#include <kfparticle_sphenix/KFParticle_sPHENIX.h>

#pragma GCC diagnostic pop

R__LOAD_LIBRARY(libkfparticle_sphenix.so)
R__LOAD_LIBRARY(libcalotrigger.so)
R__LOAD_LIBRARY(libkfparticleqa.so)

namespace HeavyFlavorReco
{
  int VERBOSITY_HF = 0;

  bool run_pipi_reco = true;
  bool run_ppi_reco = true; // set to true if needed
  bool run_KK_reco = false; // set to true if needed
  bool run_Lambdapi_reco = true; // set to true if needed
  bool run_Kpi_reco = true; // set to true if needed
  bool run_Kpipi_reco = true; // set to true if needed
  bool run_KKpi_reco = true; // set to true if needed
  bool run_pKpi_reco = true; // set to true if needed
  bool run_ee_reco = false; // set to true if needed

  std::string output_dir = "./"; //Top dir of where the output nTuples will be written
  std::string kfp_header = "outputKFParticle_";
  std::string processing_folder = "inReconstruction/";
  std::string trailer = ".root";

  // https://wiki.bnl.gov/sPHENIX/index.php/KFParticle
  std::string pipi_decay_descriptor = "K_S0 -> pi^+ pi^-"; //See twiki on how to set this
  std::string pipi_reconstruction_name = "pipi_reco"; //Used for naming output folder, file and node
  std::string pipi_output_reco_file;
  std::string pipi_output_dir;

  std::string ppi_decay_descriptor = "[Lambda0 -> proton^+ pi^-]cc"; //See twiki on how to set this
  std::string ppi_reconstruction_name = "ppi_reco"; //Used for naming output folder, file and node
  std::string ppi_output_reco_file;
  std::string ppi_output_dir;

  std::string KK_decay_descriptor = "phi -> K^+ K^-"; //See twiki on how to set this
  std::string KK_reconstruction_name = "KK_reco"; //Used for naming output folder, file and node
  std::string KK_output_reco_file;
  std::string KK_output_dir;

  std::string Lambdapi_decay_descriptor = "[Xi- -> {Lambda0 -> proton^+ pi^-} pi^-]cc"; //See twiki on how to set this
  std::string Lambdapi_reconstruction_name = "Lambdapi_reco"; //Used for naming output folder, file and node
  std::string Lambdapi_output_reco_file;
  std::string Lambdapi_output_dir;

  std::string Kpi_decay_descriptor = "[D0 -> K^- pi^+]cc"; //See twiki on how to set this
  std::string Kpi_reconstruction_name = "Kpi_reco"; //Used for naming output folder, file and node
  std::string Kpi_output_reco_file;
  std::string Kpi_output_dir;

  std::string Kpipi_decay_descriptor = "[D+ -> K^- pi^+ pi^+]cc"; //See twiki on how to set this
  std::string Kpipi_reconstruction_name = "Kpipi_reco"; //Used for naming output folder, file and node
  std::string Kpipi_output_reco_file;
  std::string Kpipi_output_dir;

  std::string KKpi_decay_descriptor = "[D_s+ -> K^+ K^- pi^+]cc"; //See twiki on how to set this
  std::string KKpi_reconstruction_name = "KKpi_reco"; //Used for naming output folder, file and node
  std::string KKpi_output_reco_file;
  std::string KKpi_output_dir;

  std::string pKpi_decay_descriptor = "[Lambda_c+ -> proton^+ K^- pi^+]cc"; //See twiki on how to set this
  std::string pKpi_reconstruction_name = "pKpi_reco"; //Used for naming output folder, file and node
  std::string pKpi_output_reco_file;
  std::string pKpi_output_dir;

  std::string ee_decay_descriptor = "[gamma -> e^+ e^-]cc"; //See twiki on how to set this
  std::string ee_reconstruction_name = "ee_reco"; //Used for naming output folder, file and node
  std::string ee_output_reco_file;
  std::string ee_output_dir;

  bool doRescale = false;
  std::string momReweightTrackMap = "SvtxTrackMapMomentumScaleCorrected";
  bool save_kfpntuple = true;
  bool use_pid = false;
  bool save_tracks_to_DST = false;
  bool dont_use_global_vertex = true;
  bool require_track_and_vertex_match = true;
  bool save_all_vtx_info = true;
  bool constrain_phi_mass = true;
  bool use_2D_matching = false;
  bool get_trigger_info = false;
  bool get_detector_info = true;
  bool get_dEdx_info = true;
  bool get_calo_info = false;
  float pid_frac = 0.4;
  float cemc_proj_radius = 102.9; //Virgile recommendation according to DetailedCalorimeterGeometry
  bool constrain_lambda_mass = true;
  bool extrapolate_tracks_to_secondary_vertex = false; // Set to false to ensure the pT map is accurate for asymmetry study

  //Light flavor used
  float lf_cuts_setMinDIRA{0.95};
  float lf_cuts_setDecayLengthRange_min{0.5};
  float lf_cuts_setMinimumTrackPV_DCA_XY{0.05};
  float lf_cuts_setMinMVTXhits{0};
  float lf_cuts_setMinINTThits{0};
  float lf_cuts_setMinTPChits{25};
  float lf_cuts_setMaximumVertexchi2nDOF{FLT_MAX};
  float lf_cuts_setMaximumDaughterDCA{0.1};

  //Unused
  float lf_cuts_setMotherPV_DCA_StdDev{FLT_MAX};
  float lf_cuts_setFlightDistancechi2{-1.};
  float lf_cuts_setMinDIRA_XY{-1.1};
  float lf_cuts_setDecayLengthRange_max{FLT_MAX};
  float lf_cuts_setDecayLengthRange_XY_min{-10.};
  float lf_cuts_setDecayLengthRange_XY_max{FLT_MAX};
  float lf_cuts_setDecayTimeRange_XY_min{-10000};
  float lf_cuts_setDecayTimeRange_XY_max{FLT_MAX};
  float lf_cuts_setDecayTimeRange_min{-10000};
  float lf_cuts_setDecayTimeRange_max{FLT_MAX};
  float lf_cuts_setMinDecayTimeSignificance{-1e5};
  float lf_cuts_setMinDecayLengthSignificance{-1e5};
  float lf_cuts_setMinDecayLengthSignificance_XY{-1e5};
  float lf_cuts_setMinimumTrackPT{0.0};
  float lf_cuts_setMinimumTrackPV_DCA{-1.};
  float lf_cuts_setMinimumTrackPV_DCA_StdDev{-1.};
  float lf_cuts_setMinimumTrackPV_DCA_StdDev_XY{-1.};
  float lf_cuts_setMaximumTrackchi2nDOF{400.};
  float lf_cuts_setMaximumDaughterDCA_XY{1};
  float lf_cuts_setMotherPT{0};
  float lf_cuts_setMaximumMotherVertexVolume{0.1};

  //Heavy flavor used
  float hf_track_to_track_DCA = 0.05;
  float hf_mother_PV_DCA = 0.02;
  float hf_daughter_PV_DCA = 0.005;
  float hf_daughter_PV_DCA_XY = 0.005;
  float hf_min_flight_distance = 0.006;
  float hf_max_flight_distance = 0.12;
  float hf_min_dira = 0.97;
  float hf_pT = 1.;

};  // namespace HeavyFlavorReco'

using namespace HeavyFlavorReco;

void init_kfp_dependencies()
{
  //dE/dx needs TRKR_CLUSTER and CYLINDERCELLGEOM_SVTX which need to be in the DST or loaded from a geo file
  Fun4AllServer *se = Fun4AllServer::instance();

  GlobalVertexReco* gblvertex = new GlobalVertexReco();
  gblvertex->Verbosity(VERBOSITY_HF);
  se->registerSubsystem(gblvertex);


  if (get_trigger_info)
  {
    TriggerRunInfoReco *triggerruninforeco = new TriggerRunInfoReco();
    se->registerSubsystem(triggerruninforeco);
  }
}

void create_hf_directories(std::string reconstruction_name, std::string &final_output_dir, std::string &output_reco_file)
{
  std::string output_file_name = kfp_header + reconstruction_name + trailer;
  final_output_dir = output_dir + reconstruction_name + "/";
  std::string output_reco_dir = final_output_dir + processing_folder;
  output_reco_file = output_reco_dir + output_file_name;

  std::string makeDirectory = "mkdir -p " + output_reco_dir;
  system(makeDirectory.c_str());
}

void reconstruct_pipi_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(pipi_reconstruction_name);
  kfparticle->Verbosity(VERBOSITY_HF);

  kfparticle->setDecayDescriptor(pipi_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(pipi_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(lf_cuts_setMotherPV_DCA_StdDev);
  kfparticle->setFlightDistancechi2(lf_cuts_setFlightDistancechi2);
  kfparticle->setMinDIRA(lf_cuts_setMinDIRA);
  kfparticle->setMinDIRA_XY(lf_cuts_setMinDIRA_XY);
  kfparticle->setDecayLengthRange(lf_cuts_setDecayLengthRange_min, lf_cuts_setDecayLengthRange_max);
  kfparticle->setDecayLengthRange_XY(lf_cuts_setDecayLengthRange_XY_min, lf_cuts_setDecayLengthRange_XY_max);
  kfparticle->setDecayTimeRange_XY(lf_cuts_setDecayTimeRange_XY_min, lf_cuts_setDecayTimeRange_XY_max);
  kfparticle->setDecayTimeRange(lf_cuts_setDecayTimeRange_min, lf_cuts_setDecayTimeRange_max);
  kfparticle->setMinDecayTimeSignificance(lf_cuts_setMinDecayTimeSignificance);
  kfparticle->setMinDecayLengthSignificance(lf_cuts_setMinDecayLengthSignificance);
  kfparticle->setMinDecayLengthSignificance_XY(lf_cuts_setMinDecayLengthSignificance_XY);

  //Track parameters
  kfparticle->setMinimumTrackPT(lf_cuts_setMinimumTrackPT);
  kfparticle->setMinimumTrackPV_DCA_StdDev(lf_cuts_setMinimumTrackPV_DCA_StdDev);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(lf_cuts_setMinimumTrackPV_DCA_StdDev_XY);
  kfparticle->setMinimumTrackPV_DCA(lf_cuts_setMinimumTrackPV_DCA);
  kfparticle->setMinimumTrackPV_DCA_XY(lf_cuts_setMinimumTrackPV_DCA_XY);
  kfparticle->setMaximumTrackchi2nDOF(lf_cuts_setMaximumTrackchi2nDOF);
  kfparticle->setMinMVTXhits(lf_cuts_setMinMVTXhits);
  kfparticle->setMinINTThits(lf_cuts_setMinINTThits);
  kfparticle->setMinTPChits(lf_cuts_setMinTPChits);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(lf_cuts_setMaximumVertexchi2nDOF);
  kfparticle->setMaximumDaughterDCA(lf_cuts_setMaximumDaughterDCA);
  kfparticle->setMaximumDaughterDCA_XY(lf_cuts_setMaximumDaughterDCA_XY);

  //Parent parameters
  kfparticle->setMotherPT(lf_cuts_setMotherPT);
  kfparticle->setMinimumMass(0.40);
  kfparticle->setMaximumMass(0.60);
  kfparticle->setMaximumMotherVertexVolume(lf_cuts_setMaximumMotherVertexVolume);
  kfparticle->setOutputName(pipi_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void reconstruct_KK_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(KK_reconstruction_name);

  kfparticle->setDecayDescriptor(KK_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(KK_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(100);
  kfparticle->setFlightDistancechi2(-1.);

  //Track parameters
  kfparticle->setMinimumTrackPT(0.1);
  kfparticle->setMaximumTrackPT(0.7);
  kfparticle->setMaximumTrackchi2nDOF(100.);
  kfparticle->setMinTPChits(25);
  kfparticle->setMinMVTXhits(1);
  kfparticle->setMinINTThits(0);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(20);
  kfparticle->setMaximumDaughterDCA(0.05);
  kfparticle->setMaximumDaughterDCA_XY(100);

  //Parent parameters
  kfparticle->setMotherPT(0);
  kfparticle->setMinimumMass(0.98);
  kfparticle->setMaximumMass(1.1);
  kfparticle->setMaximumMotherVertexVolume(0.1);

  kfparticle->setOutputName(KK_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void reconstruct_ppi_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(ppi_reconstruction_name);

  kfparticle->setDecayDescriptor(ppi_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(ppi_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(lf_cuts_setMotherPV_DCA_StdDev);
  kfparticle->setFlightDistancechi2(lf_cuts_setFlightDistancechi2);
  kfparticle->setMinDIRA(lf_cuts_setMinDIRA);
  kfparticle->setMinDIRA_XY(lf_cuts_setMinDIRA_XY);
  kfparticle->setDecayLengthRange(lf_cuts_setDecayLengthRange_min, lf_cuts_setDecayLengthRange_max);
  kfparticle->setDecayLengthRange_XY(lf_cuts_setDecayLengthRange_XY_min, lf_cuts_setDecayLengthRange_XY_max);
  kfparticle->setDecayTimeRange_XY(lf_cuts_setDecayTimeRange_XY_min, lf_cuts_setDecayTimeRange_XY_max);
  kfparticle->setDecayTimeRange(lf_cuts_setDecayTimeRange_min, lf_cuts_setDecayTimeRange_max);
  kfparticle->setMinDecayTimeSignificance(lf_cuts_setMinDecayTimeSignificance);
  kfparticle->setMinDecayLengthSignificance(lf_cuts_setMinDecayLengthSignificance);
  kfparticle->setMinDecayLengthSignificance_XY(lf_cuts_setMinDecayLengthSignificance_XY);

  //Track parameters
  kfparticle->setMinimumTrackPT(lf_cuts_setMinimumTrackPT);
  kfparticle->setMinimumTrackPV_DCA_StdDev(lf_cuts_setMinimumTrackPV_DCA_StdDev);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(lf_cuts_setMinimumTrackPV_DCA_StdDev_XY);
  kfparticle->setMinimumTrackPV_DCA(lf_cuts_setMinimumTrackPV_DCA);
  kfparticle->setMinimumTrackPV_DCA_XY(lf_cuts_setMinimumTrackPV_DCA_XY);
  kfparticle->setMaximumTrackchi2nDOF(lf_cuts_setMaximumTrackchi2nDOF);
  kfparticle->setMinMVTXhits(lf_cuts_setMinMVTXhits);
  kfparticle->setMinINTThits(lf_cuts_setMinINTThits);
  kfparticle->setMinTPChits(lf_cuts_setMinTPChits);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(lf_cuts_setMaximumVertexchi2nDOF);
  kfparticle->setMaximumDaughterDCA(lf_cuts_setMaximumDaughterDCA);
  kfparticle->setMaximumDaughterDCA_XY(lf_cuts_setMaximumDaughterDCA_XY);

  //Parent parameters
  kfparticle->setMotherPT(lf_cuts_setMotherPT);
  kfparticle->setMinimumMass(1.08);
  kfparticle->setMaximumMass(1.15);
  kfparticle->setMaximumMotherVertexVolume(lf_cuts_setMaximumMotherVertexVolume);
  kfparticle->setOutputName(ppi_output_reco_file);

  kfparticle->setOutputName(ppi_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void reconstruct_Kpi_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(Kpi_reconstruction_name);
  kfparticle->Verbosity(VERBOSITY_HF);

  kfparticle->setDecayDescriptor(Kpi_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(Kpi_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(FLT_MAX);
  kfparticle->setMotherPV_DCA(hf_mother_PV_DCA);
  kfparticle->setFlightDistancechi2(-1.);
  kfparticle->setMinDIRA(hf_min_dira);
  kfparticle->setMinDIRA_XY(-1.1);
  kfparticle->setDecayLengthRange(hf_min_flight_distance, hf_max_flight_distance);
  kfparticle->setDecayLengthRange_XY(-10., FLT_MAX);
  kfparticle->setDecayTimeRange_XY(-10000, FLT_MAX);
  kfparticle->setDecayTimeRange(-10000, FLT_MAX);
  kfparticle->setMinDecayTimeSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance_XY(-1e5);

  //Track parameters
  kfparticle->setMinimumTrackPT(0.0);
  kfparticle->setMinimumTrackPV_DCA_StdDev(-1.);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(-1.);
  kfparticle->setMinimumTrackPV_DCA(-1.);
  kfparticle->setMinimumTrackPV_DCA_XY(0.005);
  kfparticle->setMaximumTrackchi2nDOF(300.);
  kfparticle->setMinMVTXhits(1);
  kfparticle->setMinINTThits(1);
  kfparticle->setMinTPChits(20);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(FLT_MAX);
  kfparticle->setMaximumDaughterDCA(hf_track_to_track_DCA);
  kfparticle->setMaximumDaughterDCA_XY(hf_daughter_PV_DCA_XY);

  //Parent parameters
  kfparticle->setMotherPT(hf_pT);
  kfparticle->setMinimumMass(1.7);
  kfparticle->setMaximumMass(2.0);
  kfparticle->setMaximumMotherVertexVolume(0.1);
  kfparticle->setOutputName(Kpi_output_reco_file);

  kfparticle->setOutputName(Kpi_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void reconstruct_Kpipi_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(Kpipi_reconstruction_name);
  kfparticle->Verbosity(VERBOSITY_HF);

  kfparticle->setDecayDescriptor(Kpipi_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(Kpipi_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(FLT_MAX);
  kfparticle->setMotherPV_DCA(hf_mother_PV_DCA);
  kfparticle->setFlightDistancechi2(-1.);
  kfparticle->setMinDIRA(hf_min_dira);
  kfparticle->setMinDIRA_XY(-1.1);
  kfparticle->setDecayLengthRange(hf_min_flight_distance, hf_max_flight_distance);
  kfparticle->setDecayLengthRange_XY(-10., FLT_MAX);
  kfparticle->setDecayTimeRange_XY(-10000, FLT_MAX);
  kfparticle->setDecayTimeRange(-10000, FLT_MAX);
  kfparticle->setMinDecayTimeSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance_XY(-1e5);

  //Track parameters
  kfparticle->setMinimumTrackPT(0.0);
  kfparticle->setMinimumTrackPV_DCA_StdDev(-1.);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(-1.);
  kfparticle->setMinimumTrackPV_DCA(-1.);
  kfparticle->setMinimumTrackPV_DCA_XY(0.005);
  kfparticle->setMaximumTrackchi2nDOF(300.);
  kfparticle->setMinMVTXhits(1);
  kfparticle->setMinINTThits(1);
  kfparticle->setMinTPChits(20);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(FLT_MAX);
  kfparticle->setMaximumDaughterDCA(hf_track_to_track_DCA);
  kfparticle->setMaximumDaughterDCA_XY(hf_daughter_PV_DCA_XY);

  //Parent parameters
  kfparticle->setMotherPT(hf_pT);
  kfparticle->setMinimumMass(1.7);
  kfparticle->setMaximumMass(2.0);
  kfparticle->setMaximumMotherVertexVolume(0.1);
  kfparticle->setOutputName(Kpipi_output_reco_file);

  kfparticle->setOutputName(Kpipi_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void reconstruct_KKpi_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(KKpi_reconstruction_name);
  kfparticle->Verbosity(VERBOSITY_HF);

  kfparticle->setDecayDescriptor(KKpi_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(KKpi_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(FLT_MAX);
  kfparticle->setMotherPV_DCA(hf_mother_PV_DCA);
  kfparticle->setFlightDistancechi2(-1.);
  kfparticle->setMinDIRA(hf_min_dira);
  kfparticle->setMinDIRA_XY(-1.1);
  kfparticle->setDecayLengthRange(hf_min_flight_distance, hf_max_flight_distance);
  kfparticle->setDecayLengthRange_XY(-10., FLT_MAX);
  kfparticle->setDecayTimeRange_XY(-10000, FLT_MAX);
  kfparticle->setDecayTimeRange(-10000, FLT_MAX);
  kfparticle->setMinDecayTimeSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance_XY(-1e5);

  //Track parameters
  kfparticle->setMinimumTrackPT(0.0);
  kfparticle->setMinimumTrackPV_DCA_StdDev(-1.);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(-1.);
  kfparticle->setMinimumTrackPV_DCA(-1.);
  kfparticle->setMinimumTrackPV_DCA_XY(0.005);
  kfparticle->setMaximumTrackchi2nDOF(300.);
  kfparticle->setMinMVTXhits(1);
  kfparticle->setMinINTThits(1);
  kfparticle->setMinTPChits(20);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(FLT_MAX);
  kfparticle->setMaximumDaughterDCA(hf_track_to_track_DCA);
  kfparticle->setMaximumDaughterDCA_XY(hf_daughter_PV_DCA_XY);

  //Parent parameters
  kfparticle->setMotherPT(hf_pT);
  kfparticle->setMinimumMass(1.7);
  kfparticle->setMaximumMass(2.2);
  kfparticle->setMaximumMotherVertexVolume(0.1);
  kfparticle->setOutputName(KKpi_output_reco_file);

  kfparticle->setOutputName(KKpi_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void reconstruct_pKpi_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(pKpi_reconstruction_name);
  kfparticle->Verbosity(VERBOSITY_HF);

  kfparticle->setDecayDescriptor(pKpi_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(pKpi_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(FLT_MAX);
  kfparticle->setMotherPV_DCA(hf_mother_PV_DCA);
  kfparticle->setFlightDistancechi2(-1.);
  kfparticle->setMinDIRA(hf_min_dira);
  kfparticle->setMinDIRA_XY(-1.1);
  kfparticle->setDecayLengthRange(hf_min_flight_distance, hf_max_flight_distance);
  kfparticle->setDecayLengthRange_XY(-10., FLT_MAX);
  kfparticle->setDecayTimeRange_XY(-10000, FLT_MAX);
  kfparticle->setDecayTimeRange(-10000, FLT_MAX);
  kfparticle->setMinDecayTimeSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance_XY(-1e5);

  //Track parameters
  kfparticle->setMinimumTrackPT(0.0);
  kfparticle->setMinimumTrackPV_DCA_StdDev(-1.);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(-1.);
  kfparticle->setMinimumTrackPV_DCA(-1.);
  kfparticle->setMinimumTrackPV_DCA_XY(0.005);
  kfparticle->setMaximumTrackchi2nDOF(300.);
  kfparticle->setMinMVTXhits(1);
  kfparticle->setMinINTThits(1);
  kfparticle->setMinTPChits(20);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(FLT_MAX);
  kfparticle->setMaximumDaughterDCA(hf_track_to_track_DCA);
  kfparticle->setMaximumDaughterDCA_XY(hf_daughter_PV_DCA_XY);

  //Parent parameters
  kfparticle->setMotherPT(hf_pT);
  kfparticle->setMinimumMass(2.1);
  kfparticle->setMaximumMass(2.5);
  kfparticle->setMaximumMotherVertexVolume(0.1);
  kfparticle->setOutputName(pKpi_output_reco_file);

  kfparticle->setOutputName(pKpi_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void reconstruct_ee_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(ee_reconstruction_name);

  kfparticle->setDecayDescriptor(ee_decay_descriptor);

  kfparticle->extrapolateTracksToSV(extrapolate_tracks_to_secondary_vertex);

  kfparticle->saveOutput(save_kfpntuple);

  //kfparticle->usePID(use_pid);
  //kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->dontUseGlobalVertex(false);
  kfparticle->requireTrackVertexBunchCrossingMatch(false);
  kfparticle->getAllPVInfo(false);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->getCaloInfo(get_calo_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(ee_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(FLT_MAX);
  kfparticle->setFlightDistancechi2(-1.);
  kfparticle->setMinDIRA(-1.1);
  kfparticle->setMinDIRA_XY(-1.1);
  kfparticle->setDecayLengthRange(0., FLT_MAX);
  kfparticle->setDecayLengthRange_XY(-10000, FLT_MAX);
  kfparticle->setDecayTimeRange_XY(-10000, FLT_MAX);
  kfparticle->setDecayTimeRange(-10000, FLT_MAX);
  kfparticle->setMinDecayTimeSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance(-1e5);
  kfparticle->setMinDecayLengthSignificance_XY(-1e5);

  //Track parameters
  kfparticle->bunchCrossingZeroOnly(true);
  kfparticle->setMinimumTrackPT(0.2);
  kfparticle->setMaximumTrackPTchi2(FLT_MAX);
  kfparticle->setMinimumTrackPV_DCA_StdDev(-1.);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(-1.);
  kfparticle->setMinimumTrackPV_DCA(-1.);
  kfparticle->setMinimumTrackPV_DCA_XY(-100.);
  kfparticle->setMaximumTrackchi2nDOF(300.);
  kfparticle->setMinMVTXhits(0);
  kfparticle->setMinINTThits(0);
  kfparticle->setMinTPChits(25);

  //Track-Calo matching
  kfparticle->set_emcal_radius_user(cemc_proj_radius);
  //narrow window
  kfparticle->set_dphi_cut_low(-0.02); //rad
  kfparticle->set_dphi_cut_high(0.09); //rad
  kfparticle->set_dz_cut_low(-4); //cm
  kfparticle->set_dz_cut_high(4); //cm
  //loose window
  /*
  kfparticle->set_dphi_cut_low(-0.2); //rad
  kfparticle->set_dphi_cut_high(0.2); //rad
  kfparticle->set_dz_cut_low(-10); //cm
  kfparticle->set_dz_cut_high(10); //cm
  */
  kfparticle->set_emcal_e_low_cut(0.2); //GeV
  kfparticle->requireTrackEMCalMatch(true);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(FLT_MAX);
  kfparticle->setMaximumDaughterDCA(0.5);
  kfparticle->setMaximumDaughterDCA_XY(100);

  //Parent parameters
  kfparticle->setMotherPT(0);
  kfparticle->setMinimumMass(0.0);
  kfparticle->setMaximumMass(0.1);
  kfparticle->setMaximumMotherVertexVolume(0.1);

  kfparticle->setOutputName(ee_output_reco_file);

  se->registerSubsystem(kfparticle);
}

//adding xi and omega
void reconstruct_Lambdapi_mass()
{
  Fun4AllServer *se = Fun4AllServer::instance();

  KFParticle_sPHENIX *kfparticle = new KFParticle_sPHENIX(Lambdapi_reconstruction_name);
  kfparticle->Verbosity(VERBOSITY_HF);

  kfparticle->setDecayDescriptor(Lambdapi_decay_descriptor);

  kfparticle->usePID(use_pid);
  kfparticle->setPIDacceptFraction(pid_frac);
  kfparticle->get_dEdx_info();
  kfparticle->dontUseGlobalVertex(dont_use_global_vertex);
  kfparticle->requireTrackVertexBunchCrossingMatch(require_track_and_vertex_match);
  kfparticle->getAllPVInfo(save_all_vtx_info);
  kfparticle->allowZeroMassTracks();
  kfparticle->use2Dmatching(use_2D_matching);
  kfparticle->getTriggerInfo(get_trigger_info);
  kfparticle->getDetectorInfo(get_detector_info);
  kfparticle->get_dEdx_info(get_dEdx_info);
  kfparticle->saveDST(save_tracks_to_DST);
  kfparticle->setContainerName(pipi_reconstruction_name);
  kfparticle->saveParticleContainer(true);
  kfparticle->magFieldFile("FIELDMAP_TRACKING");
  if (doRescale) kfparticle->setTrackMapNodeName(momReweightTrackMap.c_str());

  //PV to SV cuts
  kfparticle->constrainToPrimaryVertex();
  kfparticle->setMotherPV_DCA_StdDev(lf_cuts_setMotherPV_DCA_StdDev);
  kfparticle->setFlightDistancechi2(lf_cuts_setFlightDistancechi2);
  kfparticle->setMinDIRA(lf_cuts_setMinDIRA);
  kfparticle->setMinDIRA_XY(lf_cuts_setMinDIRA_XY);
  kfparticle->setDecayLengthRange(0.15, lf_cuts_setDecayLengthRange_max);
  kfparticle->setDecayLengthRange_XY(lf_cuts_setDecayLengthRange_XY_min, lf_cuts_setDecayLengthRange_XY_max);
  kfparticle->setDecayTimeRange_XY(lf_cuts_setDecayTimeRange_XY_min, lf_cuts_setDecayTimeRange_XY_max);
  kfparticle->setDecayTimeRange(lf_cuts_setDecayTimeRange_min, lf_cuts_setDecayTimeRange_max);
  kfparticle->setMinDecayTimeSignificance(lf_cuts_setMinDecayTimeSignificance);
  kfparticle->setMinDecayLengthSignificance(lf_cuts_setMinDecayLengthSignificance);
  kfparticle->setMinDecayLengthSignificance_XY(lf_cuts_setMinDecayLengthSignificance_XY);

  //Track parameters
  kfparticle->setMinimumTrackPT(lf_cuts_setMinimumTrackPT);
  kfparticle->setMinimumTrackPV_DCA_StdDev(lf_cuts_setMinimumTrackPV_DCA_StdDev);
  kfparticle->setMinimumTrackPV_DCA_StdDev_XY(lf_cuts_setMinimumTrackPV_DCA_StdDev_XY);
  kfparticle->setMinimumTrackPV_DCA(lf_cuts_setMinimumTrackPV_DCA);
  kfparticle->setMinimumTrackPV_DCA_XY(lf_cuts_setMinimumTrackPV_DCA_XY);
  kfparticle->setMaximumTrackchi2nDOF(lf_cuts_setMaximumTrackchi2nDOF);
  kfparticle->setMinMVTXhits(lf_cuts_setMinMVTXhits);
  kfparticle->setMinINTThits(lf_cuts_setMinINTThits);
  kfparticle->setMinTPChits(lf_cuts_setMinTPChits);

  //Vertex parameters
  kfparticle->setMaximumVertexchi2nDOF(lf_cuts_setMaximumVertexchi2nDOF);
  kfparticle->setMaximumDaughterDCA(lf_cuts_setMaximumDaughterDCA);
  kfparticle->setMaximumDaughterDCA_XY(lf_cuts_setMaximumDaughterDCA_XY);

  //Parent parameters
  kfparticle->setMotherPT(0);
  kfparticle->setMinimumMass(1.30); //Check mass ranges
  kfparticle->setMaximumMass(1.35);
  kfparticle->setMaximumMotherVertexVolume(0.1);

  //Intermediate parameters
  kfparticle->constrainIntermediateMasses(constrain_lambda_mass);

  std::vector<std::pair<float, float>> intermediate_mass_range;
  intermediate_mass_range.push_back(make_pair(1.1057, 1.1257));
  kfparticle->setIntermediateMassRange(intermediate_mass_range);

  std::vector<float> intermediate_min_pt = {0.0};
  kfparticle->setIntermediateMinPT(intermediate_min_pt);

  std::vector<std::pair<float, float>> intermediate_PV_DCA_range;
  intermediate_PV_DCA_range.push_back(make_pair(-1., FLT_MAX));
  kfparticle->setIntermediatePV_DCARange(intermediate_PV_DCA_range);

  std::vector<std::pair<float, float>> intermediate_PV_DCAchi2_range;
  intermediate_PV_DCAchi2_range.push_back(make_pair(0., 400.));
  kfparticle->setIntermediatePV_DCA_StdDevRange(intermediate_PV_DCAchi2_range);

  std::vector<float> intermediate_min_dira = {0.90};
  kfparticle->setIntermediateMinDIRA(intermediate_min_dira);

  std::vector<float> intermediate_min_FDchi2 = {-1.};
  kfparticle->setIntermediateMinFDchi2(intermediate_min_FDchi2);

  std::vector<float> intermediate_max_vertex_vol = {1.1};
  kfparticle->setIntermediateMaxVertexVolume(intermediate_max_vertex_vol);

  kfparticle->setOutputName(Lambdapi_output_reco_file);

  se->registerSubsystem(kfparticle);
}

void end_kfparticle(std::string full_file_name, std::string final_path)
{
  ifstream file(full_file_name.c_str());
  if (file.good())
  {
    string moveOutput = "mv " + full_file_name + " " + final_path;
    system(moveOutput.c_str());
  }
}
