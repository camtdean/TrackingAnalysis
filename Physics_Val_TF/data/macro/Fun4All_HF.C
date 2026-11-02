#include <fun4all/Fun4AllUtils.h>
#include <G4_ActsGeom.C>
#include <G4_Global.C>
#include <G4_Magnet.C>
#include <G4_Mbd.C>
#include <GlobalVariables.C>
#include <Trkr_Clustering.C>
#include <Trkr_LaserClustering.C>
#include <Trkr_Reco.C>
#include <Trkr_RecoInit.C>
#include <Trkr_TpcReadoutInit.C>
#include <Calo_Calib.C>

#include <ffamodules/CDBInterface.h>

#include <fun4all/Fun4AllDstInputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllRunNodeInputManager.h>
#include <fun4all/Fun4AllServer.h>

#include <phool/recoConsts.h>
#include <phgarfield/PHGarfield.h>
#include <tpcconditions/TpcConditionsReco.h>

#include <tpctrackreco/TpcCrossingFinder.h>
#include <tpctrackreco/TpcPolyClusterTrkrClusterConverter.h>
#include <tpctrackreco/TpcPolyTrackSeedConverter.h>
#include <tpctrackreco/Tpc_AssembledTrackReco.h>
#include <tpctrackreco/Tpc_ModuleTrackReco.h>
#include <tpctrackreco/Tpc_PolyClusterizer.h>
#include <tpctrackreco/Tpc_PolyTrackReco.h>
#include <tpctrackreco/Tpc_PolyTrackVertexer.h>

#include <trackreco/PHActsTrackProjection.h>
#include <trackreco/DSTClusterPruning.h>

#include <caloreco/RawClusterBuilderTemplate.h>

#include <kfparticle_sphenix/KFParticle_sPHENIX.h>
#include <nnscalemap/nnscalemap.h>

#include <stdio.h>

#include "HF_selections.C"

using namespace HeavyFlavorReco;

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libffamodules.so)
R__LOAD_LIBRARY(libphool.so)
R__LOAD_LIBRARY(libcdbobjects.so)
R__LOAD_LIBRARY(libPHGarfield.so)
R__LOAD_LIBRARY(libtpctrackreco.so)
R__LOAD_LIBRARY(libNNScaleMap.so)

bool IsListFile = false;
bool IsDstFile = false;
bool DoUnpacking = false;
bool DoClustering = false;
bool DoSeeding = false;
bool DoFitting = false;

void CheckDstType(const std::string inputDST)
{
    if (inputDST.find("DST_STREAMING_EVENT") != std::string::npos
     || inputDST.find("DST_COMBINED") != std::string::npos)
    {
        DoUnpacking = true;
        DoClustering = true;
        DoSeeding = true;
        DoFitting = true;
    }
    else if (inputDST.find("DST_TRKR_SEED") != std::string::npos)
    {
        DoUnpacking = false;
        DoClustering = false;
        DoSeeding = false;
        DoFitting = true;
    }
    else if (inputDST.find("DST_TRKR_TRACKS") != std::string::npos)
    {
        DoUnpacking = false;
        DoClustering = false;
        DoSeeding = false;
        DoFitting = false;
    }
    return;
}

std::string GetRunInfo(const std::string &filename)
{
    const std::vector<std::string> runspecies_patterns = {"run2pp", "run3pp", "run3auau", "run3oo"};

    boost::char_separator<char> sep("_");
    boost::tokenizer<boost::char_separator<char>> tok(filename, sep);

    for (const auto &t : tok)
    {
        for (const auto &pattern : runspecies_patterns)
        {
            if (t == pattern)
            {
                return t;
            }
        }
    }

    std::cout << "Cannot extract run info from filename!" << std::endl;
    return "";
}

void Fun4All_HF(const int nEvents = 500,
                //const std::string inputDST = "DST_COMBINED_run3pp_ana571_2026p002_v001-00079516-00000.root",
                const std::string inputDST = "DST_TRKR_TRACKS_run3pp_ana573_2026p003_v001-00079516-00000.root",
                const std::string outDir = "./",
                const int nSkip = 0,
                const bool convertSeeds = false)
{
    auto se = Fun4AllServer::instance();
    se->Verbosity(1);
    // se->VerbosityDownscale(500);
    auto rc = recoConsts::instance();

    if (inputDST.find(".list") != std::string::npos)
    {
        IsListFile = true;
        std::cout << "Input is list file" << std::endl;
    }
    if (inputDST.find(".root") != std::string::npos)
    {
        IsDstFile = true;
        std::cout << "Input is dst file" << std::endl;
    }
    if (!IsListFile && !IsDstFile)
    {
        std::cout << "Check your input! Exit" << std::endl;
        gSystem->Exit(0);
    }

    int runnumber = std::numeric_limits<int>::quiet_NaN();
    int segment = std::numeric_limits<int>::quiet_NaN();
    std::string runspecies = "";

    if (IsListFile)
    {
        std::ifstream ifs(inputDST);
        std::string filepath;
        int i = 0;
        while (std::getline(ifs, filepath))
        {
            std::cout << "Adding DST with filepath: " << filepath << std::endl;
            if (i == 0)
            {
                std::pair<int, int> runseg = Fun4AllUtils::GetRunSegment(filepath);
                runnumber = runseg.first;
                segment = runseg.second;
                runspecies = GetRunInfo(filepath);
                CheckDstType(filepath);
            }
            std::string inputname = "InputManager" + std::to_string(i);
            auto *hitsin = new Fun4AllDstInputManager(inputname);
            hitsin->fileopen(filepath);
            se->registerInputManager(hitsin);
            i++;
        }
    }

    if (IsDstFile)
    {
        std::pair<int, int> runseg = Fun4AllUtils::GetRunSegment(inputDST);
        runnumber = runseg.first;
        segment = runseg.second;
        runspecies = GetRunInfo(inputDST);

        CheckDstType(inputDST);

        auto *hitsin = new Fun4AllDstInputManager("InputManager");
        hitsin->fileopen(inputDST);
        se->registerInputManager(hitsin);
    }

    rc->set_IntFlag("RUNNUMBER", runnumber);
    rc->set_IntFlag("RUNSEGMENT", segment);
    rc->set_uint64Flag("TIMESTAMP", runnumber);

    Enable::CDB = true;
    rc->set_StringFlag("CDB_GLOBALTAG", "newcdbtag");// 2026p002_v001
    rc->set_uint64Flag("TIMESTAMP", runnumber);

    std::stringstream nice_runnumber;
    nice_runnumber << std::setw(8) << std::setfill('0') << to_string(runnumber);

    int rounded_up = 100 * (std::ceil((float)(runnumber+0.1) / 100));
    std::stringstream nice_rounded_up;
    nice_rounded_up << std::setw(8) << std::setfill('0') << to_string(rounded_up);

    int rounded_down = 100 * (std::floor((float)(runnumber+0.1) / 100));
    std::stringstream nice_rounded_down;
    nice_rounded_down << std::setw(8) << std::setfill('0') << to_string(rounded_down);

    std::stringstream nice_segment;
    nice_segment << std::setw(5) << std::setfill('0') << to_string(segment);

    std::stringstream nice_skip;
    nice_skip << std::setw(5) << std::setfill('0') << to_string(nSkip);

    if (get_calo_info)
    {
        int ratio_nCalo_over_nTrkr = 100; // number of events per calo DST divided by number of events per tracking DST
        std::stringstream nice_segment_calo;
        nice_segment_calo << std::setw(5) << std::setfill('0') << to_string(segment / ratio_nCalo_over_nTrkr);

        std::string calo_anacdbver = "pro001_pcdb001_v001";
        std::string calo_file = "";
        calo_file = "/sphenix/lustre01/sphnxpro/production/" + runspecies + "/physics/" + calo_anacdbver + "/DST_CALOFITTING/run_" + nice_rounded_down.str() + "_" + nice_rounded_up.str() + "/DST_CALOFITTING_" + runspecies + "_" + calo_anacdbver + "-" + nice_runnumber.str() + "-" + nice_segment_calo.str() + ".root";
        std::cout << "Input calo DST: " << calo_file << std::endl;

        auto hitsincalo = new Fun4AllDstInputManager("CaloInputManager");
        hitsincalo->fileopen(calo_file);
        se->registerInputManager(hitsincalo);
    }

    std::string geofile = CDBInterface::instance()->getUrl("Tracking_Geometry");

    Fun4AllRunNodeInputManager *ingeo = new Fun4AllRunNodeInputManager("GeoIn");
    ingeo->AddFile(geofile);
    se->registerInputManager(ingeo);

    // set flags
    TRACKING::streaming_mode = true;

    Enable::MVTX_APPLYMISALIGNMENT = true;
    ACTSGEOM::mvtx_applymisalignment = Enable::MVTX_APPLYMISALIGNMENT;

    // distortion calibration mode
    /*
     * set to true to enable residuals in the TPC with
     * TPC clusters not participating to the ACTS track fit
     */
    G4TRACKING::SC_CALIBMODE = false;

    TpcReadoutInit(runnumber);
    // these lines show how to override the drift velocity and time offset values set in TpcReadoutInit
    // G4TPC::tpc_drift_velocity_reco = 0.0073844; // cm/ns
    // TpcClusterZCrossingCorrection::_vdrift = G4TPC::tpc_drift_velocity_reco;
    // G4TPC::tpc_tzero_reco = -5*50;  // ns
    std::cout << " run: " << runnumber << " samples: " << TRACKING::reco_tpc_maxtime_sample << " pre: " << TRACKING::reco_tpc_time_presample << " vdrift: " << G4TPC::tpc_drift_velocity_reco << std::endl;

    G4TPC::REJECT_LASER_EVENTS = true;
    // Flag for running the tpc hit unpacker with zero suppression on
    TRACKING::tpc_zero_supp = true;

    G4TPC::sampa_tzero_bias = 0;

    TRACKING::use_slim_tracks = true;

    G4TPC::ENABLE_MODULE_EDGE_CORRECTIONS = false;

    // to turn on the default static corrections, enable the two lines below
    G4TPC::ENABLE_STATIC_CORRECTIONS = false;
    G4TPC::USE_PHI_AS_RAD_STATIC_CORRECTIONS = false;

    // to turn on the average corrections, enable the three lines below
    // note: these are designed to be used only if static corrections are also applied
    G4TPC::ENABLE_AVERAGE_CORRECTIONS = false;
    G4TPC::USE_PHI_AS_RAD_AVERAGE_CORRECTIONS = false;
    // to use a custom file instead of the database file:
    G4TPC::average_correction_filename = CDBInterface::instance()->getUrl("TPC_LAMINATION_FIT_CORRECTION");

    G4MAGNET::magfield_rescale = 1;
    TrackingInit();

    output_dir = "./"; // Top dir of where the output nTuples will be written
    trailer = "_" + nice_runnumber.str() + "_" + nice_segment.str() + "_" + nice_skip.str() + ".root";

    if (DoUnpacking)
    {
        for (int felix = 0; felix < 6; felix++)
        {
            Mvtx_HitUnpacking(std::to_string(felix));
        }
        for (int server = 0; server < 8; server++)
        {
            Intt_HitUnpacking(std::to_string(server));
        }
        std::ostringstream ebdcname;
        if (runspecies == "run3pp" || runspecies == "run3auau" || runspecies == "run3oo")
        {
            for (int ebdc = 0; ebdc < 24; ebdc++)
            {
                for (int endpoint = 0; endpoint < 2; endpoint++)
                {
                    ebdcname.str("");
                    if (ebdc < 10)
                    {
                        ebdcname << "0";
                    }
                    ebdcname << ebdc << "_" << endpoint;
                    Tpc_HitUnpacking(ebdcname.str());
                }
            }
        }
        if (runspecies == "run2pp")
        {
            for (int ebdc = 0; ebdc < 24; ebdc++)
            {
                ebdcname.str("");
                if (ebdc < 10)
                {
                    ebdcname << "0";
                }
                ebdcname << ebdc;
                Tpc_HitUnpacking(ebdcname.str());
            }
        }

        Micromegas_HitUnpacking();
    }

    if (DoClustering)
    {
        Mvtx_Clustering();

        Intt_Clustering();

        Tpc_LaserEventIdentifying();

        Micromegas_Clustering();

        Reject_Laser_Events();
    }

    if (DoSeeding)
    {
        Tracking_Reco_SiliconSeed_run2pp();

        auto *converter = new TrackSeedTrackMapConverter("SiliconSeedToSvtxTrackMap");
        converter->setTrackSeedName("SiliconTrackSeedContainer");
        converter->setTrackMapName("SiliconSvtxTrackMap");
        converter->setClusterMapName("TRKR_CLUSTER");
        se->registerSubsystem(converter);

        auto *finder_svx = new PHSimpleVertexFinder("SiliconVertexFinder");
        finder_svx->Verbosity(0);
        finder_svx->setDcaCut(0.5);
        finder_svx->setTrackPtCut(0.1);
	finder_svx->setBeamSpotCutX(-0.2, 0.2);
	finder_svx->setBeamSpotCutY(-0.2, 0.2);
	finder_svx->setRequireMVTX(true);
        finder_svx->setNmvtxRequired(2);
	finder_svx->setRequireINTT(true);
	finder_svx->setNinttRequired(1);
        finder_svx->setTrackQualityCut(10000000000.0);
        finder_svx->setOutlierPairCut(0.75);
        finder_svx->setTrackMapName("SiliconSvtxTrackMap");
        finder_svx->setVertexMapName("SiliconSvtxVertexMap");
        se->registerSubsystem(finder_svx);

        se->registerSubsystem(new TpcConditionsReco());
        // Shared drift model for the crossing finder and the polyclusterizer;
        // must be registered after TpcConditionsReco and before the polyseeding modules.
        auto *garfield = new PHGarfield();
        se->registerSubsystem(garfield);

        se->registerSubsystem(new Tpc_ModuleTrackReco());     // makes TPC_MODULETRACKS
        se->registerSubsystem(new Tpc_AssembledTrackReco());  // makes TPC_ASSEMBLEDTRACKS

        auto *crossingFinder = new TpcCrossingFinder();
        crossingFinder->Verbosity(0);
        crossingFinder->setInputNodeName("TPC_ASSEMBLEDTRACKS");
        crossingFinder->setOutputNodeName("TPC_CROSSING_DECISIONS");
        crossingFinder->setVertexMapNodeName("SiliconSvtxVertexMap");  // optional, configurable
        se->registerSubsystem(crossingFinder);

        auto *cluster = new Tpc_PolyClusterizer();  // makes TPC_POLYCLUSTERS
        se->registerSubsystem(cluster);

        se->registerSubsystem(new Tpc_PolyTrackReco());      // makes TPC_POLYTRACKS
        se->registerSubsystem(new Tpc_PolyTrackVertexer());  // makes TPC_POLYTRACKVERTICES

        se->registerSubsystem(new TpcPolyTrackSeedConverter());           // converts TPC_POLYTRACKS to TpcTrackSeed
        se->registerSubsystem(new TpcPolyClusterTrkrClusterConverter());  // converts TPC_POLYCLUSTERS to TRKR_CLUSTER

        Tracking_Reco_TrackMatching_run2pp();

        auto *clusterPruner = new DSTClusterPruning("DSTClusterPruning");
        clusterPruner->pruneAllSeeds();
        se->registerSubsystem(clusterPruner);

        auto convertertpc = new TrackSeedTrackMapConverter("TpcSeedConverter");
        // Default set to full SvtxTrackSeeds. Can be set to
        // SiliconTrackSeedContainer or TpcTrackSeedContainer
        convertertpc->setTrackSeedName("TpcTrackSeedContainer");
        convertertpc->setTrackMapName("TpcSvtxTrackMap");
        convertertpc->setFieldMap(G4MAGNET::magfield_tracking);
        convertertpc->Verbosity(0);
        se->registerSubsystem(convertertpc);

        auto findertpc = new PHSimpleVertexFinder("TpcSimpleVertexFinder");
        findertpc->Verbosity(0);
        findertpc->setDcaCut(0.5);
        findertpc->setTrackPtCut(0.2);
        findertpc->setBeamLineCut(1);
        findertpc->setTrackQualityCut(1000000000);
        // findertpc->setNmvtxRequired(3);
        findertpc->setRequireMVTX(false);
        findertpc->setOutlierPairCut(0.1);
        findertpc->setTrackMapName("TpcSvtxTrackMap");
        findertpc->setVertexMapName("TpcSvtxVertexMap");
        se->registerSubsystem(findertpc);
    }

    if (DoFitting)
    {
        Tracking_Reco_SiTpcTrackMatching_run2pp("TRKR_CLUSTER_SEED");
        Tracking_Reco_TrackFit_run2pp("", "TRKR_CLUSTER_SEED");
        Tracking_Reco_Vertex_run2pp("TRKR_CLUSTER_SEED");
    }

    if (get_calo_info)
    {
        std::cout << "Begin my calo reco" << std::endl;

        Process_Calo_Calib();

        Global_Reco();

        bool doEMcalRadiusCorr = true;
        auto projection = new PHActsTrackProjection("CaloProjection");
        if (doEMcalRadiusCorr)
        {
            projection->setLayerRadius(SvtxTrack::CEMC, cemc_proj_radius);
        }
        se->registerSubsystem(projection);

        //////////////////
        // Clusters
        std::cout << "Building clusters" << std::endl;
        RawClusterBuilderTemplate *ClusterBuilder = new RawClusterBuilderTemplate("EmcRawClusterBuilderTemplate");
        ClusterBuilder->Detector("CEMC");
        ClusterBuilder->set_UseDetailedGeometry(true);
        ClusterBuilder->set_threshold_energy(0.070); // for when using basic calibration
        std::string emc_prof = getenv("CALIBRATIONROOT");
        emc_prof += "/EmcProfile/CEMCprof_Thresh30MeV.root";
        // ClusterBuilder->set_UseAltZVertex(3); //0: GlobalVertexMap, 1: MbdVertexMap, 2: Nothing, 3: G4TruthInfo
        ClusterBuilder->LoadProfile(emc_prof);
        ClusterBuilder->set_UseTowerInfo(1); // to use towerinfo objects rather than old RawTower
        se->registerSubsystem(ClusterBuilder);
    }

    if (doRescale)
    {
      std::string momReweightFile = "/sphenix/u/cdean/analysis/NNScaleMap/run" + to_string(runnumber) + "/kappa_lookup.csv";

      auto* momScale = new NNScaleMap("NNScaleMap");
      momScale->setUseCDB(false);  // NNScaleMap defaults to CDB lookup; use the fixed local file instead
      momScale->setKappaLookupFile(momReweightFile.c_str());
      momScale->setOutputTrackMapName(momReweightTrackMap.c_str());
      momScale->setScaleCovariance(true);
      se->registerSubsystem(momScale);
    }

    output_dir = outDir;

    if (run_pipi_reco) create_hf_directories(pipi_reconstruction_name, pipi_output_dir, pipi_output_reco_file);
    if (run_KK_reco) create_hf_directories(KK_reconstruction_name, KK_output_dir, KK_output_reco_file);
    if (run_ppi_reco) create_hf_directories(ppi_reconstruction_name, ppi_output_dir, ppi_output_reco_file);
    if (run_Kpi_reco) create_hf_directories(Kpi_reconstruction_name, Kpi_output_dir, Kpi_output_reco_file);
    if (run_Kpipi_reco) create_hf_directories(Kpipi_reconstruction_name, Kpipi_output_dir, Kpipi_output_reco_file);
    if (run_KKpi_reco) create_hf_directories(KKpi_reconstruction_name, KKpi_output_dir, KKpi_output_reco_file);
    if (run_pKpi_reco) create_hf_directories(pKpi_reconstruction_name, pKpi_output_dir, pKpi_output_reco_file);
    if (run_Lambdapi_reco) create_hf_directories(Lambdapi_reconstruction_name, Lambdapi_output_dir, Lambdapi_output_reco_file);
    if (run_ee_reco) create_hf_directories(ee_reconstruction_name, ee_output_dir, ee_output_reco_file);

    if (run_pipi_reco || run_KK_reco || run_ppi_reco || run_Kpi_reco || run_Kpipi_reco || run_KKpi_reco || run_pKpi_reco || run_Lambdapi_reco || run_ee_reco) init_kfp_dependencies();

    if (run_pipi_reco) reconstruct_pipi_mass();
    if (run_KK_reco) reconstruct_KK_mass();
    if (run_ppi_reco) reconstruct_ppi_mass();
    if (run_Kpi_reco) reconstruct_Kpi_mass();
    if (run_Kpipi_reco) reconstruct_Kpipi_mass();
    if (run_KKpi_reco) reconstruct_KKpi_mass();
    if (run_pKpi_reco) reconstruct_pKpi_mass();
    if (run_Lambdapi_reco) reconstruct_Lambdapi_mass();
    if (run_ee_reco) reconstruct_ee_mass();

    se->skip(nSkip);
    se->run(nEvents);
    se->End();
    se->PrintTimer();

    if (run_pipi_reco) end_kfparticle(pipi_output_reco_file, pipi_output_dir);
    if (run_KK_reco) end_kfparticle(KK_output_reco_file, KK_output_dir);
    if (run_ppi_reco) end_kfparticle(ppi_output_reco_file, ppi_output_dir);
    if (run_Kpi_reco) end_kfparticle(Kpi_output_reco_file, Kpi_output_dir);
    if (run_Kpipi_reco) end_kfparticle(Kpipi_output_reco_file, Kpipi_output_dir);
    if (run_KKpi_reco) end_kfparticle(KKpi_output_reco_file, KKpi_output_dir);
    if (run_pKpi_reco) end_kfparticle(pKpi_output_reco_file, pKpi_output_dir);
    if (run_Lambdapi_reco) end_kfparticle(Lambdapi_output_reco_file, Lambdapi_output_dir);
    if (run_ee_reco) end_kfparticle(ee_output_reco_file, ee_output_dir);

    delete se;

    std::cout << "Finished" << std::endl;
    gSystem->Exit(0);
}
