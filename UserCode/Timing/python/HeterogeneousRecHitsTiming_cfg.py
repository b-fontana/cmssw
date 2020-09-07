import os, sys, glob
import FWCore.ParameterSet.Config as cms
from Configuration.StandardSequences.Eras import eras
from Configuration.ProcessModifiers.gpu_cff import gpu
from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import HGCalRecHit

#arguments parsing
from FWCore.ParameterSet.VarParsing import VarParsing
F = VarParsing('analysis')
F.register('withGPU',
           1,
           F.multiplicity.singleton,
           F.varType.bool,
           "Whether to run with GPUs or CPUs.")
F.parseArguments()
print("********************")
print("Input arguments:")
for k,v in F.__dict__["_singletons"].items():
    print("{}: {}".format(k,v))
    print("********************")

#package loading
process = cms.Process("gpuValidation", gpu) 
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('Configuration.StandardSequences.MagneticField_cff')
#process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load('Configuration.Geometry.GeometryExtended2026D49Reco_cff')
process.load('HeterogeneousCore.CUDAServices.CUDAService_cfi')
process.load('RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi')
process.load('SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )

#indir = '/eos/cms/store/group/dpg_hgcal/comm_hgcal/bfontana/GPUScintillator/CloseByParticleGunProducer_bfontana_PDGId22_nPart1_E60_eta1p4to4p0_CE_H_Coarse_Scint_Delta_2p5_20200728_bfontana_PDGId22_nPart1_E60_eta1p4to4p0_CE_H_Coarse_Scint_Delta_2p5_20200728/RECO/'
#file_wildcard = 'closeby_PDGid22_x100_E60.0To60.0_RECO_*.root'
"""
indir = '/eos/cms/store/group/dpg_hgcal/comm_hgcal/bfontana/GPUScintillator/'
file_wildcard = 'step3_0.root'
glob = glob.glob( os.path.join(indir, file_wildcard) )
fNames = ['file:' + it for it in glob][:]
"""
#fNames = ['file:/afs/cern.ch/user/b/bfontana/CMSSW_11_2_0_pre5/src/23234.0_TTbar_14TeV+2026D49+TTbar_14TeV_TuneCP5_GenSimHLBeamSpot14+DigiTrigger+RecoGlobal+HARVESTGlobal/step3.root']
indir = '/eos/user/b/bfontana/Samples/GPU/'
fNames = [os.path.join('file:'+indir, 'step3_ttbar_D49_1120pre1_20Events_3.root')]
keep = 'keep *'
drop = 'drop CSCDetIdCSCALCTPreTriggerDigiMuonDigiCollection_simCscTriggerPrimitiveDigis__HLT'
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(fNames),
                            inputCommands = cms.untracked.vstring([keep, drop]),
                            duplicateCheckMode = cms.untracked.string("noDuplicateCheck"))

wantSummaryFlag = True
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool( wantSummaryFlag ) ) #add option for edmStreams

process.ThroughputService = cms.Service( "ThroughputService",
                                         eventRange = cms.untracked.uint32( 300 ),
                                         eventResolution = cms.untracked.uint32( 1 ),
                                         printEventSummary = cms.untracked.bool( wantSummaryFlag ),
                                         enableDQM = cms.untracked.bool( False )
                                         #valid only for enableDQM=True
                                         #dqmPath = cms.untracked.string( "HLT/Throughput" ),
                                         #timeRange = cms.untracked.double( 60000.0 ),
                                         #dqmPathByProcesses = cms.untracked.bool( False ),
                                         #timeResolution = cms.untracked.double( 5.828 )
)
process.MessageLogger.categories.append('ThroughputService')

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
                                              rcorr          	     = cms.vdouble( HGCalRecHit.__dict__['thicknessCorrection'][0:3] ),
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
                                               rcorr                   = cms.vdouble( HGCalRecHit.__dict__['thicknessCorrection'][3:6] ),
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
process.HGCalRecHits = HGCalRecHit.clone() #CPU version

if F.withGPU:
    process.recHitsTask = cms.Task( process.HeterogeneousHGCalEERecHits, process.HeterogeneousHGCalHEFRecHits, process.HeterogeneousHGCalHEBRecHits )
else:
    process.recHitsTask = cms.Task( process.HGCalRecHits )
process.path = cms.Path( process.recHitsTask )

process.out = cms.OutputModule( "PoolOutputModule", 
                                fileName = cms.untracked.string( os.path.join(indir, 'out.root') ) )
process.outpath = cms.EndPath(process.out)
