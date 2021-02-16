#include <iostream>
#include <memory>
#include <chrono>
#include <cuda_runtime.h>

#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHit.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHitCollections.h"
#include "DataFormats/ForwardDetId/interface/HGCSiliconDetId.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/EDPutToken.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
#include "Geometry/HGCalCommonData/interface/HGCalDDDConstants.h"
#include "Geometry/HGCalCommonData/interface/HGCalWaferIndex.h"

#include "HeterogeneousCore/CUDACore/interface/ScopedContext.h"
#include "HeterogeneousCore/CUDACore/interface/ContextState.h"
#include "HeterogeneousCore/CUDAServices/interface/CUDAService.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"

#include "HeterogeneousHGCRecHitMemAllocations.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "RecoLocalCalo/HGCalRecProducers/plugins/KernelManagerHGCalRecHit.h"

#include "CUDADataFormats/HGCal/interface/HGCRecHitGPUProduct.h"

class EERecHitGPUtoSoA : public edm::stream::EDProducer<edm::ExternalWork> {
public:
  explicit EERecHitGPUtoSoA(const edm::ParameterSet& ps);
  ~EERecHitGPUtoSoA() override;

  void acquire(edm::Event const&, edm::EventSetup const&, edm::WaitingTaskWithArenaHolder) override;
  void produce(edm::Event&, const edm::EventSetup&) override;

private:
  cms::cuda::ContextState ctxState_;
  edm::EDGetTokenT<cms::cuda::Product<HGCRecHitGPUProduct>> recHitGPUToken_;
  edm::EDPutTokenT<HGCRecHitSoA> recHitCPUSoAToken_;

  std::unique_ptr<HGCRecHitSoA> recHitsSoA_;
};

EERecHitGPUtoSoA::EERecHitGPUtoSoA(const edm::ParameterSet& ps)
  : recHitGPUToken_{consumes<cms::cuda::Product<HGCRecHitGPUProduct>>(ps.getParameter<edm::InputTag>("EERecHitGPUTok"))},
    recHitCPUSoAToken_(produces<HGCRecHitSoA>()) {}

EERecHitGPUtoSoA::~EERecHitGPUtoSoA() {}

void EERecHitGPUtoSoA::acquire(edm::Event const& event,
                               edm::EventSetup const& setup,
                               edm::WaitingTaskWithArenaHolder w) {
  cms::cuda::ScopedContextAcquire ctx{event.streamID(), std::move(w)};
  const auto& gpuRecHits = ctx.get(event, recHitGPUToken_);

  recHitsSoA_ = std::make_unique<HGCRecHitSoA>();

  //_allocate memory for calibrated hits on the host
  cms::cuda::host::unique_ptr<std::byte[]> dummy_hmem = memory::allocation::calibRecHitHost(gpuRecHits.nHits(), gpuRecHits.pad(), *recHitsSoA_, ctx.stream());
  
  KernelManagerHGCalRecHit km(*recHitsSoA_, gpuRecHits.get());
  km.transfer_soa_to_host(ctx.stream());
}

void EERecHitGPUtoSoA::produce(edm::Event& event, const edm::EventSetup& setup) {
  event.put(std::move(recHitsSoA_));
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(EERecHitGPUtoSoA);
