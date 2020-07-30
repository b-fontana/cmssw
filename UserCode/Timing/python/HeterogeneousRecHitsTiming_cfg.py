import os, sys, glob
import FWCore.ParameterSet.Config as cms
from Configuration.StandardSequences.Eras import eras
from Configuration.ProcessModifiers.gpu_cff import gpu
from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import HGCalRecHit

#package loading
process = cms.Process("gpuValidation", gpu) 
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('Configuration.StandardSequences.MagneticField_cff')
#process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load('Configuration.Geometry.GeometryExtended2026D46Reco_cff')
process.load('HeterogeneousCore.CUDAServices.CUDAService_cfi')
process.load('RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi')
process.load('SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import dEdX_weights_v10
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )

#fNames = ['file:/afs/cern.ch/user/b/bfontana/CMSSW_11_1_0_pre6/src/20495.0_CloseByPGun_CE_E_Front_200um+CE_E_Front_200um_2026D41_GenSimHLBeamSpotFull+DigiFullTrigger_2026D41+RecoFullGlobal_2026D41+HARVESTFullGlobal_2026D41/step3.root']
indir = '/eos/cms/store/group/dpg_hgcal/comm_hgcal/bfontana/GPUScintillator/CloseByParticleGunProducer_bfontana_PDGId22_nPart1_E60_eta1p4to4p0_CE_H_Coarse_Scint_Delta_2p5_20200728_bfontana_PDGId22_nPart1_E60_eta1p4to4p0_CE_H_Coarse_Scint_Delta_2p5_20200728/RECO/'
glob = glob.glob( os.path.join(indir, 'closeby_PDGid22_x100_E60.0To60.0_RECO_*.root') )
fNames = ['file:' + it for it in glob][:1]

keep = 'keep *'
drop = 'drop CSCDetIdCSCALCTPreTriggerDigiMuonDigiCollection_simCscTriggerPrimitiveDigis__HLT'
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(fNames),
                            inputCommands = cms.untracked.vstring([keep, drop]),
                            duplicateCheckMode = cms.untracked.string("noDuplicateCheck"))

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool( False ) ) #add option for edmStreams

process.ThroughputService = cms.Service( "ThroughputService",
                                         eventRange = cms.untracked.uint32( 300 ),
                                         eventResolution = cms.untracked.uint32( 1 ),
                                         printEventSummary = cms.untracked.bool( True ),
                                         enableDQM = cms.untracked.bool( False )
                                         #valid only for enableDQM=True
                                         #dqmPath = cms.untracked.string( "HLT/Throughput" ),
                                         #timeRange = cms.untracked.double( 60000.0 ),
                                         #dqmPathByProcesses = cms.untracked.bool( False ),
                                         #timeResolution = cms.untracked.double( 5.828 )
)

HeterogeneousHGCalEERecHits = cms.EDProducer( 'HeterogeneousHGCalEERecHitProducer',
                                              HGCEEUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCEEUncalibRecHits'),
                                              HGCEE_keV2DIGI 	     = HGCalRecHit.__dict__['HGCEE_keV2DIGI'],
                                              minValSiPar    	     = HGCalRecHit.__dict__['minValSiPar'],
                                              maxValSiPar    	     = HGCalRecHit.__dict__['maxValSiPar'],
                                              constSiPar     	     = HGCalRecHit.__dict__['constSiPar'],
                                              noiseSiPar     	     = HGCalRecHit.__dict__['noiseSiPar'],
                                              HGCEE_fCPerMIP 	     = HGCalRecHit.__dict__['HGCEE_fCPerMIP'],
                                              HGCEE_isSiFE   	     = HGCalRecHit.__dict__['HGCEE_isSiFE'],
                                              HGCEE_noise_fC 	     = HGCalRecHit.__dict__['HGCEE_noise_fC'],
                                              HGCEE_cce      	     = HGCalRecHit.__dict__['HGCEE_cce'],
                                              rcorr          	     = HGCalRecHit.__dict__['thicknessCorrection'],
                                              weights        	     = HGCalRecHit.__dict__['layerWeights'] )

HeterogeneousHGCalHEFRecHits = cms.EDProducer( 'HeterogeneousHGCalHEFRecHitProducer',
                                               HGCHEFUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCHEFUncalibRecHits'),
                                               HGCHEF_keV2DIGI         = HGCalRecHit.__dict__['HGCHEF_keV2DIGI'],
                                               minValSiPar             = HGCalRecHit.__dict__['minValSiPar'],
                                               maxValSiPar             = HGCalRecHit.__dict__['maxValSiPar'],
                                               constSiPar              = HGCalRecHit.__dict__['constSiPar'],
                                               noiseSiPar              = HGCalRecHit.__dict__['noiseSiPar'],
                                               HGCHEF_fCPerMIP         = HGCalRecHit.__dict__['HGCHEF_fCPerMIP'],
                                               HGCHEF_isSiFE           = HGCalRecHit.__dict__['HGCHEF_isSiFE'],
                                               HGCHEF_noise_fC         = HGCalRecHit.__dict__['HGCHEF_noise_fC'],
                                               HGCHEF_cce              = HGCalRecHit.__dict__['HGCHEF_cce'],
                                               rcorr                   = HGCalRecHit.__dict__['thicknessCorrection'],
                                               weights                 = HGCalRecHit.__dict__['layerWeights'] )

HeterogeneousHGCalHEBRecHits = cms.EDProducer( 'HeterogeneousHGCalHEBRecHitProducer',
                                               HGCHEBUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCHEBUncalibRecHits'),
                                               HGCHEB_keV2DIGI         = HGCalRecHit.__dict__['HGCHEB_keV2DIGI'],
                                               HGCHEB_noise_MIP        = HGCalRecHit.__dict__['HGCHEB_noise_MIP'],
                                               HGCHEB_isSiFE           = HGCalRecHit.__dict__['HGCHEB_isSiFE'],
                                               weights                 = HGCalRecHit.__dict__['layerWeights'] )

process.HeterogeneousHGCalEERecHits = HeterogeneousHGCalEERecHits
process.HeterogeneousHGCalHEFRecHits = HeterogeneousHGCalHEFRecHits
process.HeterogeneousHGCalHEBRecHits = HeterogeneousHGCalHEBRecHits
#process.HGCalRecHits = HGCalRecHit.clone() #CPU version

#process.recHitsTask = cms.Task( process.HeterogeneousHGCalEERecHits, process.HeterogeneousHGCalHEFRecHits, process.HeterogeneousHGCalHEBRecHits )
process.recHitsTask = cms.Task( process.HeterogeneousHGCalEERecHits )
process.path = cms.Path( process.recHitsTask )

"""
process.out = cms.OutputModule( "PoolOutputModule", 
                                fileName = cms.untracked.string('out.root') )
process.outpath = cms.EndPath(process.out)
"""
